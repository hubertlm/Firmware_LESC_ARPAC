/****************************************************************************
 *
 *   Copyright (c) 2012-2022 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <px4_platform_common/module.h>
#include <px4_platform_common/log.h>
#include <arch/board/board.h>
#include <nuttx/arch.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <px4_platform_common/px4_config.h>

#include <nuttx/ioexpander/gpio.h>
#include <fcntl.h>

#include "imxrt_gpio.h"  // necessário para usar imxrt_config_gpio()
#include "../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/rt117x/imxrt117x_iomuxc.h"
#include "../../platforms/nuttx/NuttX/nuttx/arch/arm/src/imxrt/hardware/rt117x/imxrt117x_pinmux.h"



__EXPORT int gpiotoggle_main(int argc, char *argv[]);

int gpiotoggle_main(int argc, char *argv[])
{
    if (argc != 3) {
        PX4_ERR("Uso: gpiotoggle <GPIO_NAME> <0|1>");
        return -1;
    }

    int value = atoi(argv[2]);

    if (value != 0 && value != 1) {
        PX4_ERR("Valor inválido. Use 0 (baixo) ou 1 (alto).");
        return -1;
    }

    px4_arch_gpiowrite(NOSSO_GPIO, value);
    
    value = px4_arch_gpioread(NOSSO_GPIO);
    PX4_INFO("GPIO setado para %d", value);

    return 0;
}
