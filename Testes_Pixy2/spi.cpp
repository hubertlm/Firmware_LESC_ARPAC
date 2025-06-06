#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstdint>
#include <iostream>
#include <cstring> // Para memcpy
#include <vector>  

#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_MODE SPI_MODE_3
#define SPI_SPEED 2000000

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

int send(uint8_t *data, uint8_t len) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)data,
        .rx_buf = 0,
        .len = len,
        .speed_hz = SPI_SPEED,
        .delay_usecs = 0,
        .bits_per_word = 8,
    };

    int ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
    std::cout << "send(): enviado " << ret << " bytes" << std::endl;
    return ret;
}

int recv(uint8_t *buffer, uint8_t len) {

    std::vector<uint8_t> dummy(len, 0); 

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)dummy.data(), 
        .rx_buf = (unsigned long)buffer,
        .len = len,
        .speed_hz = SPI_SPEED,
        .delay_usecs = 0,
        .bits_per_word = 8,
    };

    int ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
    return ret;
}


void get_version() {
    uint8_t versionRequest[] = {0xae, 0xc1, 0x0e, 0x00};
    
    uint8_t recvBuf[32] = {0}; 

    std::cout << "Enviando comando getVersion..." << std::endl;

    if (send(versionRequest, sizeof(versionRequest)) != sizeof(versionRequest)) {
        std::cerr << "Erro no envio do comando getVersion" << std::endl;
        return;
    }

    sleep(1); // Espera 1s pela resposta (aumentei um pouco para teste, 1ms é o documentado)
    int lenAttemptToReceive = 32; 
    int lenActuallyReceived = recv(recvBuf, lenAttemptToReceive);

    if (lenActuallyReceived < 0) {
        std::cerr << "Erro na leitura SPI" << std::endl;
        return;
    }
    
    std::cout << "recv() retornou: " << lenActuallyReceived << " bytes." << std::endl;
    std::cout << "Resposta bruta recebida (primeiros " << lenActuallyReceived << " bytes): ";
    for (int i = 0; i < lenActuallyReceived; i++) {
        std::cout << std::hex << (int)recvBuf[i] << " ";
    }
    std::cout << std::dec << std::endl;

    int packet_start_offset = -1;
    const int expected_packet_len = 22; // Tamanho do pacote getVersion (2 sync + 1 type + 1 len + 2 chksum + 16 data)

    // Procura pelos sync bytes (0xaf, 0xc1) no buffer recebido
    // Garante que há espaço para um pacote completo a partir do offset encontrado
    for (int i = 0; i <= (lenActuallyReceived - expected_packet_len); ++i) {
        if (recvBuf[i] == 0xaf && recvBuf[i+1] == 0xc1) {
            packet_start_offset = i;
            break;
        }
    }

    if (packet_start_offset == -1) {
        std::cerr << "Sync bytes (0xaf, 0xc1) não encontrados na resposta." << std::endl;
        return;
    }

    std::cout << "Sync bytes (0xaf, 0xc1) encontrados no offset: " << packet_start_offset << std::endl;

    // Valida se temos bytes suficientes a partir do offset para os campos do cabeçalho
    if (packet_start_offset + 6 > lenActuallyReceived) { // 6 = sync(2) + type(1) + len(1) + checksum(2)
        std::cerr << "Dados insuficientes no buffer após encontrar sync bytes para ler o cabeçalho." << std::endl;
        return;
    }

    uint8_t packetType = recvBuf[packet_start_offset + 2];
    uint8_t dataLength = recvBuf[packet_start_offset + 3]; // Deve ser 0x10 (16) para getVersion
    uint16_t dataChecksum = ((uint16_t)recvBuf[packet_start_offset + 5] << 8) | recvBuf[packet_start_offset + 4];

    // Valida o tipo de pacote para getVersion 
    if (packetType != 0x0f) {
        std::cerr << "Tipo de pacote incorreto. Esperado: 0x0f, Recebido: 0x"
                  << std::hex << (int)packetType << std::dec << std::endl;
        return;
    }
    
    // Valida o dataLength esperado para getVersion
    if (dataLength != 16) {
        std::cerr << "Data length incorreto. Esperado: 16, Recebido: "
                  << std::dec << (int)dataLength << std::endl;
        return;
    }

    // Valida se temos bytes suficientes para o dataLength declarado
    if (packet_start_offset + 6 + dataLength > lenActuallyReceived) {
        std::cerr << "Dados insuficientes no buffer para o dataLength (" << (int)dataLength << ") especificado." << std::endl;
        return;
    }

    uint16_t calculatedChecksum = 0;
    for (int i = 0; i < dataLength; i++) {
        calculatedChecksum += recvBuf[packet_start_offset + 6 + i];
    }

    if (calculatedChecksum != dataChecksum) {
        std::cerr << "Checksum inválido: Recebido=0x" << std::hex << dataChecksum
                  << ", Calculado=0x" << calculatedChecksum << std::dec << std::endl;
        return;
    }

    // Extrai os dados usando o packet_start_offset
    uint16_t hardwareVersion = ((uint16_t)recvBuf[packet_start_offset + 7] << 8) | recvBuf[packet_start_offset + 6];
    uint8_t firmwareMajor = recvBuf[packet_start_offset + 8];
    uint8_t firmwareMinor = recvBuf[packet_start_offset + 9];
    uint16_t firmwareBuild = ((uint16_t)recvBuf[packet_start_offset + 11] << 8) | recvBuf[packet_start_offset + 10];
    
    char firmwareType[11];
    memcpy(firmwareType, &recvBuf[packet_start_offset + 12], 10);
    firmwareType[10] = '\0'; // Null-terminator

    std::cout << "\n=== Informações da Pixy2 ===" << std::endl;
    std::cout << "Hardware: 0x" << std::hex << hardwareVersion << std::dec << std::endl; // Adicionado std::dec para próxima linha
    std::cout << "Firmware: " << (int)firmwareMajor << "." 
              << (int)firmwareMinor << " (Build " << firmwareBuild << ")" << std::endl;
    std::cout << "Tipo: " << firmwareType << std::endl;
}

int main() {
    std::cout << "Iniciando comunicação com a Pixy2 via SPI..." << std::endl;

    if (spi_init() != 0) {
        return -1;
    }

    get_version();

    close(spi_fd);
    return 0;
}
