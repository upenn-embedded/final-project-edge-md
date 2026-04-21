[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/-Acvnhrq)

# Final Project

**Team Number:**

**6**

**Team Name:**

**EDGE MD**

| Team Member Name | Email Address                                                |
| ---------------- | ------------------------------------------------------------ |
| Justin Monchais  | [monchais@seas.upenn.edu](mailto:monchais@seas.upenn.edu)       |
| Damodar Pai      | [damodarp@wharton.upenn.edu](mailto:damodarp@wharton.upenn.edu) |
| Tyrone Marhguy   | [tmarhguy@seas.upenn.edu](mailto:tmarhguy@seas.upenn.edu)       |

**GitHub Repository URL:**

[https://github.com/upenn-embedded/final-project-edge-md/](https://github.com/upenn-embedded/final-project-edge-md/)

**GitHub Pages Website URL:** [for final submission]*

## Final Project Proposal

### 1. Abstract

We want to build a translator between English and Spanish that is trained on the cloud and provides inference on the edge for doctors providing services to patients. Our end product should be a box that is able to be taken to any location and then takes in audio input, dissects through noise and outputs a clear translation in the corresponding language.

### 2. Motivation

The motivation of our project is that several people in the US and in 3rd world countries require a translator to act as an intermediary between doctors and patients. Not only is this dangerous from a HIPAA standpoint but it drastically minimizes the number of patients who can help because of the limitation of doctors who know a patient’s language or a translator who knows both the doctor’s and patient’s language. We hope that with a highly accurate translator that’s arguably cheap and easy to carry around, we can help doctors work with more patients at either clinics in the US or in 3rd world countries where NGOs can send more doctors to help with translation programs.

### 3. System Block Diagram

![Block Diagram](media/blockDiagram.png)

### 4. Design Sketches

![Design Sketch](media/firstSketch.png)

### 5. Software Requirements Specification (SRS)

The software system will focus on enabling accurate and efficient English to Spanish translation using a combination of a pretrained speech recognition and translation model along with domain specific correction tools. The pipeline will consist of speech to text processing, translation, and post processing to ensure medical accuracy. A pretrained model such as Whisper or a Hugging Face translation model will be used for fast inference on the edge device, while a medical dictionary and supporting language rules will be applied to refine outputs and correct potential errors in terminology and phrasing.
System Requirement 1: The system must convert spoken English or Spanish input into text with at least 90 percent accuracy under low noise conditions, measured by comparing transcriptions to ground truth samples.

System Requirement 2: The system must translate input text between English and Spanish with at least 85 percent semantic accuracy, evaluated using a predefined set of medical phrases and sentences.

System Requirement 3: The system must apply a medical dictionary to correct key terminology, ensuring that critical medical terms are translated correctly in at least 95 percent of test cases.

System Requirement 4: The system must produce translated audio output within 15 seconds of input completion, measured as end to end latency from speech input to audio playback.

System Requirement 5: The system must handle basic conversational exchanges by maintaining context across at least two consecutive sentences without significant loss of meaning.

System Requirement 6: The system must detect low confidence or failed translations and trigger an error state or retry mechanism in at least 90 percent of such cases.

**5.1 Definitions, Abbreviations**

N/A

**5.2 Functionality**

| ID     | Description                                                                                                                                            |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| SRS-01 | Convert spoken English or Spanish input into text with at least 90% accuracy under low-noise conditions, measured against ground-truth transcriptions. |
| SRS-02 | Translate text between English and Spanish with at least 85% semantic accuracy on a predefined set of medical phrases and sentences.                   |
| SRS-03 | Apply a medical dictionary so critical medical terms are translated correctly in at least 95% of test cases.                                           |
| SRS-04 | Produce translated audio output within 15 seconds of input completion, measured end-to-end from speech input to playback.                              |
| SRS-05 | Support basic conversational exchanges by preserving meaning across at least two consecutive sentences.                                                |
| SRS-06 | Detect low-confidence or failed translations and trigger an error state or retry in at least 90% of such cases.                                        |

### 6. Hardware Requirements Specification (HRS)

The hardware system must provide clear feedback on the device’s state and support a full audio input to output pipeline. Upon a button press, the device should begin recording audio input through the microphone, convert the analog signal to digital using an ADC, process the data through the translation model, and then output the translated result by converting the signal back to analog through a speaker.

System Requirement 1: The device must use LEDs to indicate system states, including power status, active listening, processing, and error conditions such as failed or low confidence translations.
System Requirement 2: The audio input system must reliably capture clear speech in environments with minimal background noise.
System Requirement 3: The system must support real time or near real time processing, ensuring that input speech is translated and played back with minimal delay.

**6.1 Definitions, Abbreviations**

N/A

**6.2 Functionality**

| ID     | Description                                                                                                                                                  |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| HRS-01 | LEDs shall indicate system state, including power, active listening, processing, and error conditions (e.g., failed or low-confidence translations).         |
| HRS-02 | The audio input path shall capture clear speech in environments with minimal background noise (microphone and conditioning suitable for clinical-style use). |
| HRS-03 | The system shall support real-time or near-real-time operation so speech is translated and played back with minimal delay.                                   |
| HRS-04 | Tactile push buttons shall provide user control aligned with the pipeline (e.g., initiating recording and other modes) via GPIO to the STM32.                |
| HRS-05 | The audio output path shall drive a speaker from the processed translation (e.g., I2S amplifier and speaker) for intelligible playback.                      |

### 7. Bill of Materials (BOM) (updated on 4/4)

INMP441 I2S MEMS microphone breakout board
MAX98357A I2S Class-D amplifier breakout board
3W 4Ω speaker, 40mm driver

Further we’ll need a logic analyzer to debug the audio as we did in one of the previous worksheets.

### 8. Final Demo Goals

For the final demo, we will present a complete end to end demonstration of the device across several realistic use cases. This will include a single sentence translation to showcase basic functionality and latency, as well as a multi turn conversation to demonstrate the system’s ability to handle continuous interaction between a doctor and patient. In addition, we will present a failure scenario in which the system encounters a low confidence or incorrect translation, followed by a demonstration of how it detects the issue and initiates a recovery or retry process.

### 9. Sprint Planning

The project is divided into four weekly sprints leading up to the April 24 deadline.

| Milestone  | Functionality Achieved                                                                                                                                                                                    | Distribution of Work                                                                                                                                                          |
| ---------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Sprint #1  | Set up the development environment; assign roles; gather hardware components; implement a basic translation pipeline using a pretrained model (e.g., Whisper or a Hugging Face English-to-Spanish model). | Software lead focuses on model setup and testing; hardware lead gathers and verifies components; team collaborates on initial pipeline integration.                           |
| Sprint #2  | Implement and test audio input and output by integrating the microphone and speaker with the Raspberry Pi; connect the translation pipeline so spoken input produces translated output.                   | Audio lead handles microphone and speaker integration; software lead connects the translation pipeline; team tests end-to-end functionality.                                  |
| MVP Demo   | Integrate the full hardware system (STM32, LEDs, buttons); improve reliability by handling noise, refining translations with the dictionary, and ensuring stable real-time performance.                   | Team works across hardware (STM32, LEDs, buttons), software (noise, dictionary, real-time behavior), and integration testing.                                                 |
| Final Demo | Debug and polish the system; prepare demo scenarios; finalize documentation; demonstrate simple translations, continuous conversation, and recovery from failed translations.                             | One member on translation model and software; one on audio processing and integration; one on hardware and system control; regular check-ins so all components work together. |

**This is the end of the Project Proposal section. The remaining sections will be filled out based on the milestone schedule.**

## Sprint Review #1

### Last week's progress

This past week we mainly focused on setting the baseline of our communcaition between the to two boards. This was led by Damodar, where he tested the aduio transfer capabalities via SPI on the STM32

![First time connecting the boards](media/IMG_3570.JPG)

We also made us of the logic analyzer to get a better understanding of how data is truly being tranferred, and to weigh of the pros and cons of differnet protocols

![Logic analyzer displaying some basic data](media/IMG_3557.JPG)

Along with this, we had some simple audio recording attempts ( which kinda failed :( ) that let us understand how we'll encoding the raw audio file done from the microphone

### Current state of project

We're now moving towards creating a consistent stream of data from the microphone to the STM/ Data store, along with loading a baseline translation model onto the Raspberry Pi

### Next week's plan

* Working model loaded on Raspberry Pi
* Clear recoding via the microphone that can be stored and passed to the model
* Speach to Text encoding that can be displayed via the terminal for testing and debugging
* Data sets found for model training
* System testing done with a standalone battery

## Sprint Review #2

![alt text](media/IMG_3500.png)

### Last week's progress

We selected LLaMA 3.2 3B as our core translation model, which sits between the Whisper STT and Piper TTS stages in the pipeline. This model was chosen to balance clinical translation accuracy against the memory and compute constraints of the Raspberry Pi 5, where it will run inference via llama.cpp with Q4_K_M quantization. We began preparing the fine-tuning workflow using Unsloth and TRL's SFTTrainer with LoRA adapters.
On the data side, we identified and sourced several datasets to support our staged training approach:

* Monolingual medical corpora (PubMed abstracts, Scielo articles, MedlinePlus content) for continued pretraining on clinical vocabulary
* Parallel Spanish-English medical sentence pairs, augmented with terminology from our UMLS/SNOMED CT/MeSH glossary, for supervised fine-tuning
* A custom preposition-sensitive evaluation set for measuring clinical translation accuracy alongside SacreBLEU

We also made a hardware communication change, switching from an SPI master/slave configuration to UART for the STM32-to-Pi link. The SPI setup was introducing timing complications with the STM32 acting as a slave device, and UART gives us a more symmetric, predictable interface with simpler flow control. This reduces a full category of synchronization bugs we were chasing.

### Current state of project

We are working through three parallel integration tasks: stabilizing the UART communication protocol between the STM32 and Pi, wiring the translation model into the pipeline so it ingests Whisper's transcription output and produces English text, and routing that output back to the STM32 for display and TTS playback through Piper.

### Next week's plan

Our focus is on closing the loop from audio input to translated output on real hardware.

* Get UART communication fully stable and reliable end-to-end between the STM32 and Pi, with consistent framing and error handling.
* Integrate the display so that both the original Spanish transcription and the English translation render on screen simultaneously.
* Validate that Whisper.cpp is transcribing Spanish audio accurately and that Piper is producing clean, intelligible English speech output
* Run an initial quantitative evaluation of the translation model — SacreBLEU scores against our test set, accuracy checks against the medical glossary via
FlashText lookup, and results from our preposition-sensitive clinical evaluation set to identify where the model is weakest
* Use those evaluation results to prioritize the next round of fine-tuning adjustments

## MVP Demo

### 1. Video

[Video showing the pipline, translation is in the next linked video](https://docs.google.com/videos/d/17sOfWx8P6tt43unIRTZpr-duvIzOP1j-f8Yf377OOpQ/edit?usp=sharing)

### 2. Audio translation

The clip below demonstrates the end-to-end audio pipeline, where we first get the raw microphone capture fed which is fed into the translation system and the Piper TTS output.

[Another video show casing the audio files generated during our MVP Demo](https://drive.google.com/file/d/18vrqvzDlqOkMEG6KEYrlbQ28RQISYc8a/view?usp=sharing)

### 3. Images

![MVP Demo setup](media/IMG_1792.jpg)

![MVP Demo setup](media/IMG_1789.jpg)

![MVP Demo setup](media/IMG_1785.jpg)

![MVP Demo setup](media/IMG_1780.jpg)

### 4. Results

We were able to work through the major components of our project in the MVP DEMO with the components that we had access to. Given that we were still waiting for our amp, we weren't able to do output on the speaker we had but we did save the wav file for output and we were able to play it off of our computers. Further, we were able to achieve very accurate sound capture, translation, and output sound quality via piper. Through this, we believe we've gotten through most of the pipeline, but next steps are creating some type of enclosure for the system, adding the LEDs for low battary, active translation, and low confidence translation.

## Final Report

Don't forget to make the GitHub pages public website!
If you’ve never made a GitHub pages website before, you can follow this webpage (though, substitute your final project repository for the GitHub username one in the quickstart guide):  [https://docs.github.com/en/pages/quickstart](https://docs.github.com/en/pages/quickstart)

### 1. Video

### 2. Images

### 3. Results

#### 3.1 Software Requirements Specification (SRS) Results

| ID     | Description                                                                                                                                            | Validation Outcome                                                                                    |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------- |
| SRS-01 | Convert spoken English or Spanish input into text with at least 90% accuracy under low-noise conditions, measured against ground-truth transcriptions. | TBD — document method, test set, and results in the `validation` folder (e.g., logs, screenshots). |
| SRS-02 | Translate text between English and Spanish with at least 85% semantic accuracy on a predefined set of medical phrases and sentences.                   | TBD — document evaluation protocol and outcomes in the `validation` folder.                        |
| SRS-03 | Apply a medical dictionary so critical medical terms are translated correctly in at least 95% of test cases.                                           | TBD — document dictionary tests and results in the `validation` folder.                            |
| SRS-04 | Produce translated audio output within 15 seconds of input completion, measured end-to-end from speech input to playback.                              | TBD — document timing measurements in the `validation` folder.                                     |
| SRS-05 | Support basic conversational exchanges by preserving meaning across at least two consecutive sentences.                                                | TBD — document scenario tests in the `validation` folder.                                          |
| SRS-06 | Detect low-confidence or failed translations and trigger an error state or retry in at least 90% of such cases.                                        | TBD — document failure-injection tests in the `validation` folder.                                 |

#### 3.2 Hardware Requirements Specification (HRS) Results

| ID     | Description                                                                                                                                                  | Validation Outcome                                                          |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------- |
| HRS-01 | LEDs shall indicate system state, including power, active listening, processing, and error conditions (e.g., failed or low-confidence translations).         | TBD — photo or video of each indicated state in the `validation` folder. |
| HRS-02 | The audio input path shall capture clear speech in environments with minimal background noise (microphone and conditioning suitable for clinical-style use). | TBD — audio samples or analyzer captures in the `validation` folder.     |
| HRS-03 | The system shall support real-time or near-real-time operation so speech is translated and played back with minimal delay.                                   | TBD — latency notes or logs in the `validation` folder.                  |
| HRS-04 | Tactile push buttons shall provide user control aligned with the pipeline (e.g., initiating recording and other modes) via GPIO to the STM32.                | TBD — demo or scope/GPIO trace in the `validation` folder.               |
| HRS-05 | The audio output path shall drive a speaker from the processed translation (e.g., I2S amplifier and speaker) for intelligible playback.                      | TBD — playback demo or measurements in the `validation` folder.          |

### 4.1 Audio Capture Pipeline — STM32 ADC + UART

To establish a working audio input pipeline, we implemented analog microphone capture on the STM32 Nucleo-F411RE using the SPW2430 MEMS microphone connected to PA0 (ADC1 Channel 0). The STM32 continuously samples the microphone output at 12-bit resolution (0–4095) and frames each sample as a 3-byte binary packet — a `0xFF` sync byte followed by the two ADC bytes — transmitted over USART1 (PA9) at 9600 baud to the Raspberry Pi.

On the Raspberry Pi side, a Python script listens on `/dev/ttyAMA10`, reconstructs each 12-bit ADC value from the binary packets, scales it to 16-bit signed PCM, and saves 10-second recordings as standard WAV files at 8kHz. This gives us a verified, end-to-end audio capture pipeline from microphone to file, which feeds directly into the Whisper STT stage.

### 4.2 **Connections:**

| SPW2430 | STM32 | RPi |
|---------|-------|-----|
| 3V | 3.3V | — |
| GND | GND | GND |
| AC | PA0 (ADC1 CH0, CN7 Pin 28) | — |
| — | PA9 (USART1 TX, CN10 Pin 1) | Pin 10 (GPIO15 RXD) |

### 5. Conclusion

We are very close to completing this project, just waiting on the amp and need to finish the last of LED connections.

## References
