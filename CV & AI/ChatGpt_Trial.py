import openai

# Initialize the client with your API key
openai.api_key = "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"

prompt = "What's the most eaten food in the world?"

# Create a chat completion
response = openai.Completion.create(
    model="gpt-3.5-turbo",
    prompt=prompt,
    max_tokens=100
)

# Print the response
print(response.choices[0].text.strip())
