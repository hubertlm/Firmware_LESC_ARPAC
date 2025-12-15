#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstdint>
#include <iostream>
#include <cstring> // Para memcpy
#include <vector>  
#include <csignal>


#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_MODE SPI_MODE_3
#define SPI_SPEED 2000000

volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    std::cout << "\nEncerrando o programa..." << std::endl;
    running = 0;
}

int spi_fd;

int spi_init() {
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        std::cerr << "Erro ao abrir dispositivo SPI" << std::endl;
        return -1;
    }

    int mode = SPI_MODE;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        std::cerr << "Erro ao configurar modo SPI" << std::endl;
        close(spi_fd);
        return -1;
    }

    int speed = SPI_SPEED;
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        std::cerr << "Erro ao configurar velocidade SPI" << std::endl;
        close(spi_fd);
        return -1;
    }

    std::cout << "SPI inicializado com sucesso!" << std::endl;
    return 0;
}

int transfer(uint8_t *tx_buf, uint8_t *rx_buf, int len) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = (uint32_t)len,
        .speed_hz = SPI_SPEED,
        .bits_per_word = 8,
    };
    return ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}


void get_blocks() {
    // Comando: Sync(2) + Type(1) + Len(1) + Payload(2)
    // 0x20 = getBlocks, 0x02 = Len, 0xff = All sigs, 0xff = Max blocks
    uint8_t request[] = {0xae, 0xc1, 0x20, 0x02, 0xff, 0xff};
    uint8_t response[256] = {0}; // Buffer para resposta

    // Envia a solicitação
    if (transfer(request, response, sizeof(request)) < 0) {
        std::cerr << "Erro no envio SPI" << std::endl;
        return;
    }

    sleep(1); // 1s de espera

    // Lê a resposta
    // Lemos um bloco grande o suficiente para conter vários objetos
    uint8_t tx_dummy[256] = {0};
    if (transfer(tx_dummy, response, 256) < 0) {
        std::cerr << "Erro na leitura SPI" << std::endl;
        return;
    }

    // Procura pelos Sync Bytes 0xaf, 0xc1
    int packet_offset = -1;
    for (int i = 0; i < 256 - 6; ++i) {
        if (response[i] == 0xaf && response[i+1] == 0xc1) {
            packet_offset = i;
            break;
        }
    }

    if (packet_offset != -1) {
        uint16_t checksum = ((uint16_t)response[packet_offset + 5] << 8) | response[packet_offset + 4];
        uint8_t data_len = response[packet_offset + 3];
        
        uint16_t sum = 0;
        for (int i = 0; i < data_len; i++) sum += response[packet_offset + 6 + i];

        if (sum == checksum && data_len > 0) {
            int num_blocks = data_len / 14;
            std::cout << "\r[Rastreio] Objetos detectados: " << num_blocks << "   " << std::flush;
            
            // Exibe apenas o primeiro bloco para não poluir o terminal
            if (num_blocks > 0) {
                int i = 0; // Primeiro bloco
                int base = packet_offset + 6 + (i * 14);
                uint16_t x = ((uint16_t)response[base + 3] << 8) | response[base + 2];
                uint16_t y = ((uint16_t)response[base + 5] << 8) | response[base + 4];
                std::cout << "| Alvo Principal em X=" << x << ", Y=" << y << "      ";
            }
        } else {
             std::cout << "\r[Rastreio] Nenhum objeto válido.              " << std::flush;
        }
    }
}

int main() {
    // Registra o Ctrl+C para sair do loop
    signal(SIGINT, signal_handler);

    if (spi_init() != 0) return -1;
    
    std::cout << "Iniciando Loop de Rastreio (Ctrl+C para parar)..." << std::endl;

    while (running) {
        get_blocks();
        
        // Controle de Frame Rate 
        usleep(20000); 
    }

    close(spi_fd);
    std::cout << "\nConexão fechada" << std::endl;
    return 0;
}
