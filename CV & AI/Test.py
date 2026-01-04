import cohere
import pyttsx3


co = cohere.Client('5sdONErRjqtt22rIyOQ7pc3XC08GtB86UIzpgL3i')

engine = pyttsx3.init()

def speak_text(text):
    """Convert text to speech and play it."""
    engine.setProperty('rate', 180)  
    engine.setProperty('volume', 1.0) 
    engine.say(text)
    engine.runAndWait()

def generate_response(prompt):
    """Generate a response using Cohere's API."""
    
    full_prompt = "Talk like iron man assistant jarvis, I want short responses: " + prompt
    
    response = co.generate(
        model='mystery-model',  
        prompt=full_prompt, 
        max_tokens=120
    )
    return response.generations[0].text.strip()

def main():
    """Main function to interact with the user."""
    speak_text("Athletech X AI is online, What can i do for you today?")
    while True:
 
        command = input("Type your question (or 'exit' to quit): ").lower()
        
        if 'exit' in command or 'stop' in command:
            speak_text("See You Again!")
            break


        response = generate_response(command)
        
   
        print(f"Edith: {response}")
        

        speak_text(response)

if __name__ == "__main__":
    main()
