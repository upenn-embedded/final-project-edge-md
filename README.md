[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/-Acvnhrq)
# Final Project

**Team Number:** 

**6**

**Team Name:**

**EDGE MD**

| Team Member Name | Email Address       |
|------------------|---------------------|
| Justin Monchais  | monchais@seas.upenn.edu |
| Damodar Pai      | damodarp@wharton.upenn.edu |
| Tyrone Marhguy   | tmarhguy@seas.upenn.edu    |

**GitHub Repository URL:** 

https://github.com/upenn-embedded/final-project-edge-md/

**GitHub Pages Website URL:** [for final submission]*

## Final Project Proposal

### 1. Abstract

We want to build a translator between English and Spanish that is trained on the cloud and provides inference on the edge for doctors providing services to patients. Our end product should be a box that is able to be taken to any location and then takes in audio input, dissects through noise and outputs a clear translation in the corresponding language.  

### 2. Motivation

The motivation of our project is that several people in the US and in 3rd world countries require a translator to act as an intermediary between doctors and patients. Not only is this dangerous from a HIPAA standpoint but it drastically minimizes the number of patients who can help because of the limitation of doctors who know a patient’s language or a translator who knows both the doctor’s and patient’s language. We hope that with a highly accurate translator that’s arguably cheap and easy to carry around, we can help doctors work with more patients at either clinics in the US or in 3rd world countries where NGOs can send more doctors to help with translation programs. 
### 3. System Block Diagram

### 4. Design Sketches

ADD IMAGE HERE

### 5. Software Requirements Specification (SRS)

The software system will focus on enabling accurate and efficient English to Spanish translation using a combination of a pretrained speech recognition and translation model along with domain specific correction tools. The pipeline will consist of speech to text processing, translation, and post processing to ensure medical accuracy. A pretrained model such as Whisper or a Hugging Face translation model will be used for fast inference on the edge device, while a medical dictionary and supporting language rules will be applied to refine outputs and correct potential errors in terminology and phrasing.
System Requirement 1: The system must convert spoken English or Spanish input into text with at least 90 percent accuracy under low noise conditions, measured by comparing transcriptions to ground truth samples.

System Requirement 2: The system must translate input text between English and Spanish with at least 85 percent semantic accuracy, evaluated using a predefined set of medical phrases and sentences.

System Requirement 3: The system must apply a medical dictionary to correct key terminology, ensuring that critical medical terms are translated correctly in at least 95 percent of test cases.

System Requirement 4: The system must produce translated audio output within 15 seconds of input completion, measured as end to end latency from speech input to audio playback.

System Requirement 5: The system must handle basic conversational exchanges by maintaining context across at least two consecutive sentences without significant loss of meaning.

System Requirement 6: The system must detect low confidence or failed translations and trigger an error state or retry mechanism in at least 90 percent of such cases.

**5.1 Definitions, Abbreviations**

Here, you will define any special terms, acronyms, or abbreviations you plan to use for hardware

**5.2 Functionality**

| ID     | Description                                                                                                                                                                                                              |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| SRS-01 | The IMU 3-axis acceleration will be measured with 16-bit depth every 100 milliseconds +/-10 milliseconds                                                                                                                 |
| SRS-02 | The distance sensor shall operate and report values at least every .5 seconds.                                                                                                                                           |
| SRS-03 | Upon non-nominal distance detected (i.e., the trap mechanism has changed at least 10 cm from the nominal range), the system shall be able to detect the change and alert the user in a timely manner (within 5 seconds). |
| SRS-04 | Upon a request from the user, the system shall get an image from the internal camera and upload the image to the user system within 10s.                                                                                 |

### 6. Hardware Requirements Specification (HRS)

The hardware system must provide clear feedback on the device’s state and support a full audio input to output pipeline. Upon a button press, the device should begin recording audio input through the microphone, convert the analog signal to digital using an ADC, process the data through the translation model, and then output the translated result by converting the signal back to analog through a speaker.

System Requirement 1: The device must use LEDs to indicate system states, including power status, active listening, processing, and error conditions such as failed or low confidence translations.
System Requirement 2: The audio input system must reliably capture clear speech in environments with minimal background noise.
System Requirement 3: The system must support real time or near real time processing, ensuring that input speech is translated and played back with minimal delay.


**6.1 Definitions, Abbreviations**

Here, you will define any special terms, acronyms, or abbreviations you plan to use for hardware

**6.2 Functionality**

| ID     | Description                                                                                                                        |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------- |
| HRS-01 | A distance sensor shall be used for obstacle detection. The sensor shall detect obstacles at a maximum distance of at least 10 cm. |
| HRS-02 | A noisemaker shall be inside the trap with a strength of at least 55 dB.                                                           |
| HRS-03 | An electronic motor shall be used to reset the trap remotely and have a torque of 40 Nm in order to reset the trap mechanism.      |
| HRS-04 | A camera sensor shall be used to capture images of the trap interior. The resolution shall be at least 480p.                       |

### 7. Bill of Materials (BOM)

Raspberry Pi 5, 8GB RAM
Official RPi 5 27W USB-C power supply
MicroSD card, 128GB, A2 rated (Samsung EVO Plus or similar)
STM32F446RE Nucleo-64 development board 
INMP441 I2S MEMS microphone breakout board 
MAX98357A I2S Class-D amplifier breakout board
3W 4Ω speaker, 40mm driver
5 resistors, 220Ω (for LEDs)
4 resistors, 10kΩ (pull-ups for buttons)
4 LEDs of different colors 
4 tactile push buttons (12mm, through-hole for breadboard) 
Further we’ll need a logic analyzer to debug the audio as we did in one of the previous worksheets. 


### 8. Final Demo Goals

For the final demo, we will present a complete end to end demonstration of the device across several realistic use cases. This will include a single sentence translation to showcase basic functionality and latency, as well as a multi turn conversation to demonstrate the system’s ability to handle continuous interaction between a doctor and patient. In addition, we will present a failure scenario in which the system encounters a low confidence or incorrect translation, followed by a demonstration of how it detects the issue and initiates a recovery or retry process.


### 9. Sprint Planning

| Milestone  | Functionality Achieved | Distribution of Work |
| ---------- | ---------------------- | -------------------- |
| Sprint #1  |                        |                      |
| Sprint #2  |                        |                      |
| MVP Demo   |                        |                      |
| Final Demo |                        |                      |

**This is the end of the Project Proposal section. The remaining sections will be filled out based on the milestone schedule.**

## Sprint Review #1

### Last week's progress

### Current state of project

### Next week's plan

## Sprint Review #2

### Last week's progress

### Current state of project

### Next week's plan

## MVP Demo

## Final Report

Don't forget to make the GitHub pages public website!
If you’ve never made a GitHub pages website before, you can follow this webpage (though, substitute your final project repository for the GitHub username one in the quickstart guide):  [https://docs.github.com/en/pages/quickstart](https://docs.github.com/en/pages/quickstart)

### 1. Video

### 2. Images

### 3. Results

#### 3.1 Software Requirements Specification (SRS) Results

| ID     | Description                                                                                               | Validation Outcome                                                                          |
| ------ | --------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| SRS-01 | The IMU 3-axis acceleration will be measured with 16-bit depth every 100 milliseconds +/-10 milliseconds. | Confirmed, logged output from the MCU is saved to "validation" folder in GitHub repository. |

#### 3.2 Hardware Requirements Specification (HRS) Results

| ID     | Description                                                                                                                        | Validation Outcome                                                                                                      |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| HRS-01 | A distance sensor shall be used for obstacle detection. The sensor shall detect obstacles at a maximum distance of at least 10 cm. | Confirmed, sensed obstacles up to 15cm. Video in "validation" folder, shows tape measure and logged output to terminal. |
|        |                                                                                                                                    |                                                                                                                         |

### 4. Conclusion


## References

