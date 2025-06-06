#include <px4_platform_common/log.h>
#include <px4_platform_common/module.h>

extern "C" __EXPORT int lesc_app_main(int argc, char *argv[]);

int lesc_app_main(int argc, char *argv[]) {
    PX4_INFO("Aplicativo lesc_app rodando.");
    return 0;
}
