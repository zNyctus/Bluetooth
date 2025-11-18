// Por enquanto é a estrutura meio padrão do projeto. Scripts recilados do Rebonatto
// Não é muito difícil. Basicamente o UUID é a "porta" e as características são os "cômodos" da casa.
// As características essencialmente são propriedades de uma classe. A nossa classe aqui é o serviço.
// Então as características podem representar qualquer coisa: um sensor, um motor, uma bateria...
// Cada característica tem propriedades, que correspondem ao que a gente quer que essa característica realize (ler, escrever, notificar)
// O callback essencialmente é uma reação a um pedido do CLIENTE, como por exemplo "ligue o led pra mim aí"
// Notify notifica o cliente sempre que algo muda (a não ser que tenha vindo de um callback, ou seja, que o cliente PEDIU pra mudar)
// O Descriptor não viaja junto com os dados. Ele é meio que uma tag pra gente saber o que significa aquele dado, no notify.
// Confusões: 
// CARACTERISTICA3->addDescriptor(new BLE2902()); e  BLEDescriptor *NotifyDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));:
// Esses aí de cima são um tipo de descritor. Basicamente o cliente vê um botão no app na CARACTERISTICA3 onde ele pode ligar/desligar o
// fluxo que é notificado pra ele. Tipo, se tiver um loop que notifica um contador pra ele, ele pode decidir que quer parar de receber flood
// E o NotifyDescriptor->setValue("Esse aqui é o notificador."); CARACTERISTICA3->addDescriptor(NotifyDescriptor);...
// é o descritor de fato. Serve como documentação e pra facilitar a vida do dev do frontend. Na CARACTERISTA3, vai estar escrito
// "Esse aqui é o notificador", e o dev vai entender "ah, então é essa a característica que tá me floodando mensagem, né?"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <stdlib.h>

// https://www.uuidgenerator.net/

// Caracteristicazinhas BÁSICAS
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CARACTERISTICA1 "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Essa Lê
#define CARACTERISTICA2 "c5e5483e-36e1-4688-b7f5-ea07361b26a9" // Essa Escreve
#define CARACTERISTICA3 "a3a5483e-36e1-4688-b7f5-ea07361b26a0" // Essa notifica 

// Caracteristicazinhas pra parte do trabalho que pede índice
#define CARACTERISTICA_PERGUNTA "50dd0a97-646a-496a-ad06-4731d96c473f" 
#define CARACTERISTICA_RESPOSTA "b854c39b-f3d8-41b1-83a9-9990ac9f9cb9"

// Ponteirões show de bola pra gente conseguir definir as propriedades das características e basicamente poder usar elas, ali no script
BLECharacteristic *carac1_Ler;
BLECharacteristic *carac2_Escrever;
BLECharacteristic *carac3_Notif;
BLECharacteristic *caracPergunta;
BLECharacteristic *caracResposta;

int valorNotify = 0;
bool dispositivoConectado = false;

float SensorTemperatura = 30.6;
bool led = false;
float historicoTemperatura[24];

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      dispositivoConectado = true;
      Serial.println("Dispositivo Conectado");
    }

    void onDisconnect(BLEServer* pServer) {
      dispositivoConectado = false;
      Serial.println("Conexão desfeita... Voltando a anunciar");
      BLEDevice::startAdvertising(); // Reinicia o anúncio
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        String uuid = pCharacteristic->getUUID().toString().c_str();

        Serial.print("Escrita Recebida (Callback) no UUID: ");
        Serial.println(uuid);

        if (value.length() > 0) {
            Serial.print("Valor recebido: ");
            for (int i = 0; i < value.length(); i++) {
                Serial.print(value[i]);
            }
            Serial.println("\n-----------------");

            if(uuid.equals(CARACTERISTICA2))
            {
                if(value == "1")
                {
                    if(!led)
                    {
                        Serial.println("Atualizando valor:");
                        led = true;
                    }
                    else Serial.println("O led já está ligado!:");

                }
                else if (value == "0")
                {
                    if(led)
                    {
                        Serial.println("Atualizando valor:");
                        led = false;
                    }
                    else Serial.println("O led já está desligado!");
                }
                else Serial.print("Erro: use '1' ou '0'.");

                Serial.print("O estado atual do led é: "); Serial.println(led ? "LIGADO" : "DESLIGADO");
            }
            else if(uuid.equals(CARACTERISTICA_PERGUNTA))
            {
                //LÓGICA DO INDEX

                int indice = atoi(value.c_str()); // Lembrando que atoi é pra converter string pra inteiro. A gente recebe tudo em string
                Serial.print("O índice que o cliente pediu é: "); Serial.println(indice);
                
                if(indice >=0 && indice <24) // A gente vê se é válido o número que o cliente mandou pra gente (se tá dentro das últimas 24 horas)
                {
                  float temperaturaDoIndice = historicoTemperatura[indice];
                  caracResposta->setValue(String(temperaturaDoIndice).c_str());  // Já que o nosso caracResposta lê, significa que a gente usa ele pro cliente ver a temperatura

                  Serial.println("Valor atualizado.");
                }

                else caracResposta->setValue("Indice inválido");              
            }
        }
    }
};


void setup() {
    Serial.begin(9600);
    Serial.println("Iniciando Servidor BLE...");

    for(int i = 0; i < 24; i++)
    {
        historicoTemperatura[i] = 20.0 + (rand() % 150) / 10.0;
    }

    BLEDevice::init("Servidor_BLE_Felipe_Enzo");

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);


                            // CARACTERISTICA 1
    carac1_Ler = pService->createCharacteristic(
                                CARACTERISTICA1,
                                BLECharacteristic::PROPERTY_READ 
                            );
    carac1_Ler->setValue(String(SensorTemperatura).c_str());
     
    BLEDescriptor *ReadDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    ReadDescriptor->setValue("Esse daqui lê o valor da temperatura");
    carac1_Ler->addDescriptor(ReadDescriptor);



                            // CARACTERISTICA 2
    carac2_Escrever = pService->createCharacteristic(
                                CARACTERISTICA2,
                                BLECharacteristic::PROPERTY_WRITE 
                             );
    carac2_Escrever->setCallbacks(new MyCharacteristicCallbacks()); 

    BLEDescriptor *WriteDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    WriteDescriptor->setValue("Esse daqui acende/apaga o led (0 ou 1).");
    carac2_Escrever->addDescriptor(WriteDescriptor);

    
                             // CARACTERISTICA 3
    carac3_Notif = pService->createCharacteristic(
                                CARACTERISTICA3,
                                BLECharacteristic::PROPERTY_NOTIFY
                            );
    carac3_Notif->addDescriptor(new BLE2902()); // Esse camarada aqui é o nosso interruptor do fluxo. Então é um segundo descritor

    BLEDescriptor *NotifyDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    NotifyDescriptor->setValue("Esse aqui é o notificador chato.");
    carac3_Notif->addDescriptor(NotifyDescriptor);


                            // CARACTERISTICA PERGUNTA
    caracPergunta = pService->createCharacteristic(
                                CARACTERISTICA_PERGUNTA,
                                BLECharacteristic::PROPERTY_WRITE
                            );
    caracPergunta->setCallbacks(new MyCharacteristicCallbacks()); 

    BLEDescriptor *QuestionDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    QuestionDescriptor->setValue("Escreve um indice (0-23)");
    caracPergunta->addDescriptor(QuestionDescriptor);    


                            //CARACTERISTICA RESPOSTA
    caracResposta = pService->createCharacteristic(
                                CARACTERISTICA_RESPOSTA,
                                BLECharacteristic::PROPERTY_READ
                            );
    BLEDescriptor *RespostaDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    RespostaDescriptor->setValue("Leia o valor do indice pedido aqui");
    caracResposta->addDescriptor(RespostaDescriptor);                        


    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); // Aqui que ele começa a anunciar pra galera
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); 
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Servidor BLE iniciado e anunciando!");
}

void loop() {
    if (dispositivoConectado) 
    {
        valorNotify++; 
        
        carac3_Notif->setValue((uint8_t*)&valorNotify, 4);
        carac3_Notif->notify();
        
        Serial.print("Notificando valor: ");
        Serial.println(valorNotify);

        SensorTemperatura += 0.1;
        if(SensorTemperatura >= 40.0) SensorTemperatura = 25.0;

        carac1_Ler->setValue(String(SensorTemperatura).c_str());

    }
    
    delay(2000);
}