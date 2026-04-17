/*
 * Passback playback test — paste the USER CODE sections into main.c.
 *
 * Prerequisites (configure in CubeMX before using this):
 *   - USART1: RX enabled, baud = 460800, DMA on RX (circular, byte width)
 *   - I2S2 (or I2S3): Master Transmit, Philips standard, 16-bit data,
 *     16-bit or 32-bit frame, DMA on TX, sample rate = 16000 Hz
 *     (or whatever rate Piper TTS outputs — must match WAV file rate)
 *   - Pins: I2S BCK, WS, SD wired to MAX98357A BCLK, LRC, DIN
 *
 * Ping-pong buffer design:
 *   UART DMA fills one 256-sample half while I2S DMA plays the other.
 *   HAL_I2S_TxCpltCallback and HAL_I2S_TxHalfCpltCallback signal when
 *   each half finishes so UART can refill it.
 */

/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PD */
#define AUDIO_CHUNK   256          // samples per half-buffer
#define AUDIO_BUF     (AUDIO_CHUNK * 2)  // total ping-pong buffer (samples)
#define MAGIC_0  0xDE
#define MAGIC_1  0xAD
#define MAGIC_2  0xBE
#define MAGIC_3  0xEF
/* USER CODE END PD */

/* USER CODE BEGIN PV */
// Declare these in the Private Variables section alongside hi2s2 / huart1
int16_t  audio_buf[AUDIO_BUF];    // ping-pong: [0..255] and [256..511]
uint32_t wav_sample_rate;
uint32_t wav_num_samples;
volatile uint8_t  half_done = 0;  // set by I2S half-complete callback
volatile uint8_t  full_done = 0;  // set by I2S full-complete callback
/* USER CODE END PV */

/* USER CODE BEGIN 0 */

// Wait for exactly 'n' bytes from UART (blocking helper)
static void uart_recv(uint8_t *dst, uint32_t n)
{
    HAL_UART_Receive(&huart1, dst, n, HAL_MAX_DELAY);
}

// Parse the 12-byte header sent by rpi_send.py
// Returns 1 on success, 0 if magic bytes don't match
static int parse_header(void)
{
    uint8_t hdr[12];
    uart_recv(hdr, sizeof(hdr));

    if (hdr[0] != MAGIC_0 || hdr[1] != MAGIC_1 ||
        hdr[2] != MAGIC_2 || hdr[3] != MAGIC_3)
        return 0;

    wav_sample_rate = hdr[4] | (hdr[5]<<8) | (hdr[6]<<16) | (hdr[7]<<24);
    wav_num_samples = hdr[8] | (hdr[9]<<8) | (hdr[10]<<16) | (hdr[11]<<24);
    return 1;
}

/* USER CODE END 0 */


/* ---- In main(), inside USER CODE BEGIN 2 ---- */
/* USER CODE BEGIN 2 */

    // Scan for the header (retry until magic found)
    while (!parse_header()) {
        // flush one byte and retry — handles mid-stream starts
        uint8_t discard;
        HAL_UART_Receive(&huart1, &discard, 1, 100);
    }

    // Pre-fill both halves before starting I2S so DMA never starves
    uart_recv((uint8_t*)audio_buf, AUDIO_BUF * 2);  // 2 bytes per sample

    // Start I2S DMA in circular mode — it will keep playing audio_buf
    // and fire half/full callbacks each time it passes the midpoint
    HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t*)audio_buf, AUDIO_BUF);

/* USER CODE END 2 */


/* ---- In while(1), inside USER CODE BEGIN 3 ---- */
/* USER CODE BEGIN 3 */

    // Refill whichever half I2S just finished playing
    if (half_done) {
        half_done = 0;
        // I2S just finished the first half — refill [0..CHUNK-1]
        uart_recv((uint8_t*)&audio_buf[0], AUDIO_CHUNK * 2);
    }
    if (full_done) {
        full_done = 0;
        // I2S just finished the second half — refill [CHUNK..BUF-1]
        uart_recv((uint8_t*)&audio_buf[AUDIO_CHUNK], AUDIO_CHUNK * 2);
    }

/* USER CODE END 3 */


/* ---- USER CODE BEGIN 4 (after main) ---- */
/* USER CODE BEGIN 4 */

// Called when I2S DMA finishes the first half of audio_buf
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    half_done = 1;
}

// Called when I2S DMA finishes the second half of audio_buf
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    full_done = 1;
}

/* USER CODE END 4 */
