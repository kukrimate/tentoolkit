#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <archive.h>
#include <archive_entry.h>

#define PRODUCTS_URL "https://go.microsoft.com/fwlink/?LinkId=841361"
#define PRODUCTS_CAB "products.cab"
#define PRODUCTS_XML "products.xml"

static int isfile(const char *path)
{
	int isfile = 0;
	FILE *fp = fopen(path, "r");
	if (fp) {
		isfile = 1;
		fclose(fp);
	}
	return isfile;
}

static int download_file(const char *url, const char *path)
{
	CURL *curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "Failed to create libcurl handle\n");
		return -1;
	}

	FILE *fp = fopen(path, "w");
	if (!fp) {
		perror(path);
		curl_easy_cleanup(curl);
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	CURLcode r;
	if ((r = curl_easy_perform(curl)) != CURLE_OK) {
		fprintf(stderr, "%s: %s\n", url, curl_easy_strerror(r));
		fclose(fp);
		remove(path);
		curl_easy_cleanup(curl);
		return -1;
	}

	fclose(fp);
	curl_easy_cleanup(curl);
	return 0;
}

static int decompress_file(const char *archive_path, const char *filename)
{
	struct archive *archive = archive_read_new();
	if (!archive) {
		fprintf(stderr, "Failed to create libarchive handle\n");
		return -1;
	}

	archive_read_support_format_all(archive);
	archive_read_support_filter_all(archive);
	if (archive_read_open_filename(archive, archive_path, 16384)) {
		perror(archive_path);
		goto err1;
	}

	int r;
	struct archive_entry *entry;

	for (;;) {
		r = archive_read_next_header(archive, &entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r != ARCHIVE_OK) {
			perror(archive_path);
			goto err1;
		}
		if (!strcmp(archive_entry_pathname(entry), filename))
			goto extract_file;
	}

	fprintf(stderr, "%s: File not found in archive\n", filename);
	goto err1;

extract_file:;
	FILE *fp = fopen(filename, "w");

	if (!fp) {
		perror(filename);
		goto err1;
	}

	for (char buffer[4096];;) {
		r = archive_read_data(archive, buffer, sizeof(buffer));
		if (r < 0) {
			perror(archive_path);
			fclose(fp);
			goto err1;
		}
		if (!r)
			break;
		if (!fwrite(buffer, 1, r, fp) && !feof(fp)) {
			perror(filename);
			fclose(fp);
			goto err1;
		}
	}

	fclose(fp);

	archive_read_free(archive);
	return 0;
err1:
	archive_read_free(archive);
	return -1;
}

static xmlNode *walkXml(xmlNode *node, const char *path)
{
	char *next = strchr(path, '/');

	for (; node; node = node->next) {
		if (!strncmp(node->name, path, strchrnul(path, '/') - path))
			if (!next)
				return node;
			else
				return walkXml(node->children, next + 1);
	}

	return NULL;
}

typedef struct _esd_file esd_file;

struct _esd_file
{
	const char *name;
	const char *url;
	const char *arch;
	const char *lang;
	int is_business;

	esd_file *next;
};

static void remove_duplicate(esd_file **headptr)
{
	for (esd_file *t, **i = headptr; *i; i = &(*i)->next) {
		for (esd_file *j = *headptr; j; j = j->next) {
			if (*i != j && !strcmp((*i)->name, j->name)) {
				t = (*i)->next;
				free(*i);
				*i = t;
			}
		}
	}
}

static int rflag = 0, uflag = 0, bflag = 0;
static char *filter_lang = NULL, *filter_arch = NULL;

int main(int argc, char *argv[])
{
	char ch;

	while (-1 != (ch = getopt(argc, argv, "hrbul:a:")))
		switch (ch) {
		case 'h':
			fprintf(stderr,
				"Usage: esdurl [-h] [-r] [-u] [-b] [-l lang] [-a arch]\n");
			return EXIT_FAILURE;
		case 'r':
			rflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'l':
			filter_lang = optarg;
			break;
		case 'a':
			filter_arch = optarg;
			break;
		case '?':
			return EXIT_FAILURE;
		default:
			abort();
		}

	// libcurl init
	if (curl_global_init(CURL_GLOBAL_ALL)) {
		perror("Failed to init curl");
		return EXIT_FAILURE;
	}

	// libxml2 init
	LIBXML_TEST_VERSION

	// download products.xml from Microsoft
	if (!isfile(PRODUCTS_XML) || rflag &&
		(-1 == download_file(PRODUCTS_URL, PRODUCTS_CAB) ||
		-1 == decompress_file(PRODUCTS_CAB, PRODUCTS_XML)))
		goto err1;
	remove(PRODUCTS_CAB);

	// parse products.xml
	xmlDocPtr doc = xmlReadFile(PRODUCTS_XML, NULL, 0);
	if (!doc) {
		fprintf(stderr, "Failed to parse products file\n");
		goto err1;
	}

	xmlNode *files = walkXml(xmlDocGetRootElement(doc),
		"MCT/Catalogs/Catalog/PublishedMedia/Files");
	if (!files) {
		fprintf(stderr, "Invalid XML file\n");
		goto err2;
	}

	esd_file *head = NULL, **nextptr = &head;

	for (xmlNode *file = files->children; file; file = file->next) {
		if (!strcmp(file->name, "File")) {
			*nextptr = calloc(1, sizeof(esd_file));

			for (xmlNode *node = file->children; node; node = node->next) {
				if (!strcmp(node->name, "FileName")) {
					(*nextptr)->name = xmlNodeGetContent(node);
				}
				else if (!strcmp(node->name, "FilePath")) {
					(*nextptr)->url = xmlNodeGetContent(node);
					if (strstr((*nextptr)->url, "BUSINESS"))
						(*nextptr)->is_business = 1;
				}
				else if (!strcmp(node->name, "LanguageCode")) {
					(*nextptr)->lang = xmlNodeGetContent(node);
				}
				else if (!strcmp(node->name, "Architecture")) {
					(*nextptr)->arch = xmlNodeGetContent(node);
				}
			}

			if (!(*nextptr)->name || !(*nextptr)->url
					|| !(*nextptr)->lang || !(*nextptr)->arch) {
				free(*nextptr);
				continue;
			}
			nextptr = &(*nextptr)->next;
		}
	}

	if (!head) {
		fprintf(stderr, "No ESD files were found\n");
		goto err2;
	}

	remove_duplicate(&head);

	for (esd_file *t, *esd = head; esd; ) {
		if (filter_lang && strcmp(esd->lang, filter_lang))
			goto no_print;
		if (filter_arch && strcmp(esd->arch, filter_arch))
			goto no_print;
		if (bflag && !esd->is_business)
			goto no_print;

		if (uflag)
			printf("%s\n", esd->url);
		else
			printf("Name: %s\nArch: %s\nLang: %s\nBusiness: %d\nURL: %s\n\n",
				esd->name, esd->arch, esd->lang, esd->is_business, esd->url);

		no_print:
		t = esd->next;
		free(esd);
		esd = t;
	}

	xmlFreeDoc(doc);
	xmlCleanupParser();
	curl_global_cleanup();
	return EXIT_SUCCESS;
err2:
	xmlFreeDoc(doc);
err1:
	xmlCleanupParser();
	curl_global_cleanup();
	return EXIT_FAILURE;
}
