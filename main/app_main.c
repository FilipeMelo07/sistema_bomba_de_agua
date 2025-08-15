/*
 * Projeto Final Completo - Bomba D'água com Controle Automático e Manual
 *
 * Versão com controle de limites de distância pela Web.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h> 
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

static const char *TAG = "BOMBA_CONTROL_FINAL";

// --- DEFINIÇÃO DOS PINOS ---
#define MODO_INDICATOR_PIN      GPIO_NUM_2
#define PINO_RELE               GPIO_NUM_12
#define PINO_LED_MQTT_OK        GPIO_NUM_14
#define PINO_LED_RELE_STATUS    GPIO_NUM_27
#define PINO_TRIGGER            GPIO_NUM_26
#define PINO_ECHO               GPIO_NUM_25

// --- TÓPICOS MQTT ---
#define MODO_CONTROL_TOPIC      "esp32/comando/modo"
#define RELE_CONTROL_TOPIC      "esp32/comando/rele"
#define LIGAR_DIST_CONTROL_TOPIC  "esp32/comando/dist_ligar"
#define DESLIGAR_DIST_CONTROL_TOPIC "esp32/comando/dist_desligar"

#define MODO_STATUS_TOPIC       "esp32/status/modo"
#define DISTANCIA_STATUS_TOPIC  "esp32/status/distancia"
#define RELE_STATUS_TOPIC       "esp32/status/bomba" 

// --- VARIÁVEIS DE ESTADO E CONTROLE ---
static float DISTANCIA_LIGAR_BOMBA   = 60.0f;
static float DISTANCIA_DESLIGAR_BOMBA = 20.0f;

static bool modo_automatico = true;
static bool rele_esta_ligado = false;
static esp_mqtt_client_handle_t client = NULL;

// --- FUNÇÕES DE PUBLICAÇÃO DE STATUS ---
void publish_current_mode() {
    if (client) {
        const char *mode_str = modo_automatico ? "AUTOMATICO" : "MANUAL";
        esp_mqtt_client_publish(client, MODO_STATUS_TOPIC, mode_str, 0, 1, 0);
    }
}

void publish_distance_status(const char* distance_str) {
    if (client) {
        esp_mqtt_client_publish(client, DISTANCIA_STATUS_TOPIC, distance_str, 0, 1, 0);
    }
}

void publish_current_pump_status() {
    if (client) {
        const char* status_bomba = rele_esta_ligado ? "LIGADA" : "DESLIGADA";
        esp_mqtt_client_publish(client, RELE_STATUS_TOPIC, status_bomba, 0, 1, 0);
    }
}

void set_pump_state(bool ligar) {
    gpio_set_level(PINO_RELE, ligar);
    gpio_set_level(PINO_LED_RELE_STATUS, ligar);
    rele_esta_ligado = ligar;
    publish_current_pump_status(); // Publica a mudança de estado
}

/* --- TAREFA DO SENSOR --- */
void sensor_task(void *pvParameters)
{
    gpio_reset_pin(PINO_TRIGGER);
    gpio_set_direction(PINO_TRIGGER, GPIO_MODE_OUTPUT);
    gpio_reset_pin(PINO_ECHO);
    gpio_set_direction(PINO_ECHO, GPIO_MODE_INPUT);
    ESP_LOGI("SENSOR_TASK", "Tarefa do Sensor iniciada.");
    bool sensor_pausado_log = !modo_automatico;

    while(1) {
        if (modo_automatico) {
            if (sensor_pausado_log) {
                ESP_LOGI("SENSOR_TASK", "Modo automático ativado. Retomando leituras.");
                sensor_pausado_log = false;
            }

            gpio_set_level(PINO_TRIGGER, 0); esp_rom_delay_us(2);
            gpio_set_level(PINO_TRIGGER, 1); esp_rom_delay_us(10);
            gpio_set_level(PINO_TRIGGER, 0);

            uint32_t startTime = esp_timer_get_time();
            while(gpio_get_level(PINO_ECHO) == 0 && (esp_timer_get_time() - startTime) < 500000);
            startTime = esp_timer_get_time();
            while(gpio_get_level(PINO_ECHO) == 1 && (esp_timer_get_time() - startTime) < 500000);
            long duration = esp_timer_get_time() - startTime;
            
            float distanciaCm = (duration * 0.0343) / 2.0;
            char distancia_str[16];

            if (distanciaCm > 2 && distanciaCm < 400) {
                snprintf(distancia_str, sizeof(distancia_str), "%.1f", distanciaCm);
                publish_distance_status(distancia_str);

                if (!rele_esta_ligado && distanciaCm > DISTANCIA_LIGAR_BOMBA) {
                    set_pump_state(true);
                }
                else if (rele_esta_ligado && distanciaCm < DISTANCIA_DESLIGAR_BOMBA) {
                    set_pump_state(false);
                }
            } else {
                publish_distance_status("Fora de alcance");
            }
        } else {
            if (!sensor_pausado_log) {
                ESP_LOGW("SENSOR_TASK", "Modo manual ativado. Pausando leituras.");
                publish_distance_status("Sensor inativo (modo manual)");
                sensor_pausado_log = true;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}



/* --- LÓGICA MQTT --- */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT CONECTADO");
        gpio_set_level(PINO_LED_MQTT_OK, 1);
        esp_mqtt_client_subscribe(client, MODO_CONTROL_TOPIC, 0);
        esp_mqtt_client_subscribe(client, RELE_CONTROL_TOPIC, 0);
        esp_mqtt_client_subscribe(client, LIGAR_DIST_CONTROL_TOPIC, 0);
        esp_mqtt_client_subscribe(client, DESLIGAR_DIST_CONTROL_TOPIC, 0);
        
        ESP_LOGI(TAG, "Enviando status inicial para o broker...");
        publish_current_mode();
        publish_current_pump_status();
        if(modo_automatico) {
            publish_distance_status("Aguardando leitura...");
        } else {
            publish_distance_status("Sensor inativo (modo manual)");
        }
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT DESCONECTADO");
        gpio_set_level(PINO_LED_MQTT_OK, 0);
        break;

    case MQTT_EVENT_DATA:
    { 
        
        ESP_LOGI(TAG, "Dados recebidos, Tópico: %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "Mensagem: %.*s", event->data_len, event->data);

        char data_str[event->data_len + 1];
        memcpy(data_str, event->data, event->data_len);
        data_str[event->data_len] = '\0';

        if (strncmp(event->topic, MODO_CONTROL_TOPIC, event->topic_len) == 0) {
            modo_automatico = (strncmp(data_str, "AUTOMATICO", event->data_len) == 0);
            gpio_set_level(MODO_INDICATOR_PIN, modo_automatico);
            publish_current_mode();
        }
        else if (strncmp(event->topic, RELE_CONTROL_TOPIC, event->topic_len) == 0) {
            if (!modo_automatico) {
                if (strncmp(data_str, "LIGAR", event->data_len) == 0) set_pump_state(true);
                else if (strncmp(data_str, "DESLIGAR", event->data_len) == 0) set_pump_state(false);
            }
        }
        else if (strncmp(event->topic, LIGAR_DIST_CONTROL_TOPIC, event->topic_len) == 0) {
            float novo_valor = atof(data_str);
            if (novo_valor > 0) {
                DISTANCIA_LIGAR_BOMBA = novo_valor;
                ESP_LOGW(TAG, "Limite para LIGAR a bomba atualizado para: %.2f cm", DISTANCIA_LIGAR_BOMBA);
            }
        }
        else if (strncmp(event->topic, DESLIGAR_DIST_CONTROL_TOPIC, event->topic_len) == 0) {
            float novo_valor = atof(data_str);
            if (novo_valor > 0) {
                DISTANCIA_DESLIGAR_BOMBA = novo_valor;
                ESP_LOGW(TAG, "Limite para DESLIGAR a bomba atualizado para: %.2f cm", DISTANCIA_DESLIGAR_BOMBA);
            }
        }
        break;
    }
    
    default:
        // Este case default lida com todos os outros eventos não especificados.
        break;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Inicializando...");
    gpio_config_t io_conf = { .mode = GPIO_MODE_OUTPUT, .pin_bit_mask = (1ULL << MODO_INDICATOR_PIN) | (1ULL << PINO_RELE) | (1ULL << PINO_LED_MQTT_OK) | (1ULL << PINO_LED_RELE_STATUS) };
    gpio_config(&io_conf);

    gpio_set_level(MODO_INDICATOR_PIN, 1);
    set_pump_state(false); 
    gpio_set_level(PINO_LED_MQTT_OK, 0);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());    
    ESP_ERROR_CHECK(example_connect());
    
    esp_mqtt_client_config_t mqtt_cfg = { .broker.address.uri = CONFIG_BROKER_URL, .network.disable_auto_reconnect = false };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    
    xTaskCreate(&sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}