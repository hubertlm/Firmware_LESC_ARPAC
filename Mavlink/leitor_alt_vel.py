from pymavlink import mavutil
import time

# Conectar à Cube Orange via UART
master = mavutil.mavlink_connection('/dev/ttyAMA0', baud=57600)

# Esperar pelo primeiro heartbeat para confirmar a conexão
master.wait_heartbeat()
print("Conectado ao veículo!")

# Loop principal para leitura de dados
while True:
    # Ler todas as mensagens disponíveis
    msg = master.recv_match(blocking=True)
    
    if msg:
        msg_type = msg.get_type()
        
        if msg_type == "GPS_RAW_INT":
            velocidade = msg.vel / 100.0  # Convertendo de cm/s para m/s
            print(f"Velocidade do veículo: {velocidade:.2f} m/s")
        
        elif msg_type == "ALTITUDE":
            altitude = getattr(msg, 'bottom_clearance', None)  # Pode ser None se não existir
            
            if altitude is not None:
                print(f"Distância do veículo até o solo: {altitude:.2f} m")
            else:
                print("Campo 'bottom_clearance' não disponível na mensagem ALTITUDE.")

    # Aguardar 1 segundo antes de buscar novos dados
    time.sleep(1)
