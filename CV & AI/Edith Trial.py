from gtts import gTTS
from playsound import playsound
import os
import speech_recognition as sr
import re
import numpy as np

def speak_text(text):
    tts = gTTS(text, lang='en')
    filename = "response.mp3"
    tts.save(filename)
    playsound(filename)
    os.remove(filename)

def get_voice_input(prompt="Listening..."):
    recognizer = sr.Recognizer()
    with sr.Microphone() as source:
        print(prompt)
        audio = recognizer.listen(source)
        try:
            text = recognizer.recognize_google(audio)
            print(f"User said: {text}")
            return text.lower()
        except sr.UnknownValueError:
            print("Sorry, I did not understand that.")
            return ""

def wait_for_activation():
    while True:
        command = get_voice_input("Say 'Hello Edith' to activate.")
        if "hello edith" in command:
            speak_text("How can I assist you?")
            return True

def process_command(command):
    if any(op in command for op in ['add', 'plus', 'sum']):
        numbers = [int(num) for num in re.findall(r'\d+', command)]
        result = np.sum(numbers)
        speak_text(f"The result is {result}")
    elif any(op in command for op in ['subtract', 'minus']):
        numbers = [int(num) for num in re.findall(r'\d+', command)]
        result = numbers[0] - numbers[1]
        speak_text(f"The result is {result}")
    elif any(op in command for op in ['multiply', 'times']):
        numbers = [int(num) for num in re.findall(r'\d+', command)]
        result = np.prod(numbers)
        speak_text(f"The result is {result}")
    elif any(op in command for op in ['divide']):
        numbers = [int(num) for num in re.findall(r'\d+', command)]
        if numbers[1] != 0:
            result = numbers[0] / numbers[1]
            speak_text(f"The result is {result}")
        else:
            speak_text("Division by zero is not allowed.")
    else:
        speak_text("I can only perform basic arithmetic operations like addition, subtraction, multiplication, and division.")

def main():
    speak_text("Edith is active")
    while True:
        if wait_for_activation():
            command = get_voice_input()
            if 'exit' in command or 'stop' in command:
                speak_text("Goodbye!")
                break
            process_command(command)

if __name__ == "__main__":
    main()
