#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <wimlib.h>

// names for the setup media and boot.wim images respectively
#define MEDIA_NAME "Windows Setup Media"
#define WPE_NAME "Microsoft Windows PE"
#define WSETUP_NAME "Microsoft Windows Setup"

// include images with this installation type in install.esd
#define INSTALLATIONTYPE "Client"

int main(int argc, const char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: mkmedia <esd_path> <media_path>\n");
		return EXIT_FAILURE;
	}

	if (wimlib_global_init(0)) {
		fprintf(stderr, "Failed to initialize wimlib");
		return EXIT_FAILURE;
	}

	// check media directory
	if (mkdir(argv[2], 0775)) {
		perror(argv[2]);
		goto fail;
	}

	WIMStruct *src, *bootWim, *installEsd;

	if (wimlib_open_wim(argv[1], WIMLIB_OPEN_FLAG_CHECK_INTEGRITY, &src)) {
		fprintf(stderr, "Failed to open ESD\n");
		goto fail;
	}

	struct wimlib_wim_info src_info;
	wimlib_get_wim_info(src, &src_info);

	int i_media = -1, i_wpe = -1, i_wsetup = -1;

	for (size_t i = 1; i <= src_info.image_count; ++i) {
		if (!strncmp(wimlib_get_image_name(src, i), MEDIA_NAME,
				strlen(MEDIA_NAME)))
			i_media = i;
		if (!strncmp(wimlib_get_image_name(src, i), WPE_NAME,
				strlen(WPE_NAME)))
			i_wpe = i;
		if (!strncmp(wimlib_get_image_name(src, i), WSETUP_NAME,
				strlen(WSETUP_NAME)))
			i_wsetup = i;
	}

	if (i_media == -1 || i_wpe == -1 || i_wsetup == -1) {
		fprintf(stderr, "ESD file is not a valid Windows release\n");
		goto fail2;
	}

	// extract media
	if (wimlib_extract_image(src, i_media, argv[2], 0)) {
		fprintf(stderr, "Failed to extract setup media\n");
		goto fail2;
	}

	char pathbuf[PATH_MAX];

	// create boot.wim
	if (wimlib_create_new_wim(WIMLIB_COMPRESSION_TYPE_LZX, &bootWim)) {
		fprintf(stderr, "Failed to create boot.wim\n");
		goto fail2;
	}
	snprintf(pathbuf, PATH_MAX, "%s/sources/%s", argv[2], "boot.wim");
	if (wimlib_export_image(src, i_wpe, bootWim, 0, 0, 0)
			|| wimlib_export_image(src, i_wsetup, bootWim, 0, 0, WIMLIB_EXPORT_FLAG_BOOT)
			|| wimlib_write(bootWim, pathbuf, WIMLIB_ALL_IMAGES, 0, 0)) {
		fprintf(stderr, "Failed to export boot.wim\n");
		goto fail3;
	}

	// create install.esd
	if (wimlib_create_new_wim(WIMLIB_COMPRESSION_TYPE_LZMS, &installEsd)) {
		fprintf(stderr, "Failed to create install.esd\n");
		goto fail3;
	}

	const char *install_type = NULL;

	for (size_t i = 1; i <= src_info.image_count; ++i) {
		install_type = wimlib_get_image_property(src, i, "WINDOWS/INSTALLATIONTYPE");
		if (install_type &&
				!strncmp(install_type, INSTALLATIONTYPE, strlen(INSTALLATIONTYPE))) {
			if (wimlib_export_image(src, i, installEsd, 0, 0, 0)) {
				fprintf(stderr, "Failed to export install.esd\n");
				goto fail4;
			}
		}
	}
	snprintf(pathbuf, PATH_MAX, "%s/sources/%s", argv[2], "install.esd");
	if (wimlib_write(installEsd, pathbuf, WIMLIB_ALL_IMAGES, 0, 0)) {
		fprintf(stderr, "Failed to export install.esd\n");
		goto fail4;
	}

	wimlib_free(installEsd);
	wimlib_free(bootWim);
	wimlib_free(src);
	wimlib_global_cleanup();
	return EXIT_SUCCESS;
fail4:
	wimlib_free(installEsd);
fail3:
	wimlib_free(bootWim);
fail2:
	wimlib_free(src);
fail:
	wimlib_global_cleanup();
	return EXIT_FAILURE;
}
