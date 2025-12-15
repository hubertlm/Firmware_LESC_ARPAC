#include <px4_platform_common/log.h>
#include <px4_platform_common/module.h>

extern "C" __EXPORT int lesc_app_main(int argc, char *argv[]);

int lesc_app_main(int argc, char *argv[])
{
    PX4_INFO("Aplicativo do Lesc!");
 
    int vehicle_sub = orb-subscribe(ORB_ID(vheicle_attitude));
    struct vehicle_attitude_s attitude;

    int battery_sub = orb_subscribe(ORB_ID(battery_status));
    struct battery_status_s battery;
 
 
    if(orb_copy(ORB_ID(vehicle_attitude), vehicle_sub, &attitude) == 0{
        PX4_INFO("altitude do veiculo: %.2fvV", (double) attitude.pitch);
    }    
    else{
        PX4_ERR("Erro ao ler a altitude do veiculo");
    }
    
    if (orb_copy(ORB_ID(battery_status), battery_sub, &battery) == 0) {
        PX4_INFO("Tensao da bateria: %.2fV", (double) battery.voltage_v);
    } else {
        PX4_ERR("Erro ao ler a tensão da bateria!");
    }

    orb_unsubscribe(vehicle_sub);
    orb_unsubscribe(battery_sub);

    return 0;
}