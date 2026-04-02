import chromadb 
import os
import re
import io
import tiktoken
import numpy as np 
import base64 
from openai import OpenAI  
from langchain_community.embeddings import OpenAIEmbeddings 
from chromadb.config import Settings  # Import the Settings class
from PIL import Image

MAX_TOKENS = 16384   

# Specify the directory where you want to store the ChromaDB data 
chroma_db_directory = "C:/EXAMPLES/AI/ChromaDBStorages/cad_commands"
 
collection_name = "cad_commands"

# Create the Settings object with the persist_directory
db_settings = Settings(persist_directory=chroma_db_directory, is_persistent=True) 

# Initialize ChromaDB client with the settings object
client_db = chromadb.Client(settings=db_settings)
 
# Open AI
llm_model="gpt-5-chat-latest"  
client_ai = OpenAI(  api_key=os.environ.get("OPENAI_API_KEY") ) 

# CPT4all
# llm_model="Qwen2-1.5B-Instruct"
# llm_model="Llama 3.1 8B Instruct 128k"
# client_ai = OpenAI(    base_url="http://localhost:4891/v1",    api_key="" )

text_embedding_model = "text-embedding-3-small"
# text_embedding_model = "text-embedding-3-large"

# --------------------------------------------------------------------------------------------------
# retrieve_relevant_files
# --------------------------------------------------------------------------------------------------
def retrieve_relevant_files(query, top_k=20, metadata_filter=None):
    """
    Retrieves relevant documents from ChromaDB based on a query and optional metadata filter.

    Parameters:
        query (str): The search query.
        top_k (int): Number of top similar documents to retrieve.
        metadata_filter (dict): Optional metadata filter (e.g., {"parent_uuid": "xyz123"}).

    Returns:
        list: Relevant document contents.
    """
    print(f"Retrieveing ChromaDB collection '{collection_name}'")  
    
    collection = client_db.get_or_create_collection(collection_name)
    

    # Generate the query embedding vector
    embedding_model = OpenAIEmbeddings(model=text_embedding_model)
    query_embedding = embedding_model.embed_query(query)

    relevant_files = []

    try:
        results = collection.query(
            query_embeddings=[query_embedding],
            n_results=top_k,
            where=metadata_filter,  # <-- filter goes here
            include=["documents", "metadatas", "distances"]
        )
    except Exception as e:
        print("Query failed:", e)
        return []

    for doc, meta, dist in zip(results["documents"][0], results["metadatas"][0], results["distances"][0]):
        relevant_files.append({"content": doc, "metadata": meta, "distance": dist})
        print(f"Content:  {doc}")
        print(f"Metadata: {meta}")
        print(f"Distance: {dist}\n")

    return relevant_files
# --------------------------------------------------------------------------------------------------
# retrieve_relevant_files

 
# --------------------------------------------------------------------------------------------------
# get_token_length
# --------------------------------------------------------------------------------------------------
def get_token_length(input_text):
    encoding = tiktoken.get_encoding("cl100k_base")  # Encoding for GPT models
    tokens = encoding.encode(input_text)
    return len(tokens)
# --------------------------------------------------------------------------------------------------
# get_token_length
  
# --------------------------------------------------------------------------------------------------
# truncate_input
# --------------------------------------------------------------------------------------------------
def truncate_input(input_text, max_tokens=4096):
    token_length = get_token_length(input_text)


    if token_length > max_tokens:
        print(f"Input is too long ({token_length} tokens). Truncating...")
        encoding = tiktoken.get_encoding("cl100k_base")
        tokens = encoding.encode(input_text)
        truncated_tokens = tokens[
            :max_tokens
        ]  # Keep only the first 'max_tokens' tokens
        truncated_tokens = tokens[
            :max_tokens
        ]  # Keep only the first 'max_tokens' tokens
        truncated_text = encoding.decode(truncated_tokens)  # Decode back to text
        return truncated_text
    else:
        return input_text
# --------------------------------------------------------------------------------------------------
# truncate_input

# --------------------------------------------------------------------------------------------------
# split_into_chunks
# --------------------------------------------------------------------------------------------------
def split_into_chunks(input_text, chunk_size=3500):
    chunks = []
    tokens = input_text.split()  # Split text into words or tokens
    for i in range(0, len(tokens), chunk_size):
        chunk = " ".join(tokens[i : i + chunk_size])
        chunk = " ".join(tokens[i : i + chunk_size])
        chunks.append(chunk)
    return chunks
# --------------------------------------------------------------------------------------------------
# split_into_chunks

# --------------------------------------------------------------------------------------------------
# instruction
# --------------------------------------------------------------------------------------------------
instruction = (
    "You are an AI agent with extensive expertise in the interpretation of CAA components in JSON format. "
    "Your role is to determine the appropriate CAA component based on a user’s natural language input — which may be given in English, German, French, or other languages — "
    "by searching the provided JSON structure for the most relevant match.\n"
    "\n"
    "Your analysis should focus on component attributes such as 'name', 'description', 'command', and 'function'. "
    "Give preference to entries with the function value 'CAA-Components' over 'CATIA-Commands' when both are relevant.\n"
    "\n"
    "You will use the following JSON context to guide your answer:\n"
    "{{context}}\n"
    "\n"
    "Respond to the following user request:\n"
    "{{question}}\n"
    "\n"
    "Rules:\n"
    "1. Analyze the meaning of the user's input.\n"
    "2. Search the JSON structure (provided as 'context') for a matching component.\n"
    "3. Match by name, description, command, or function — prioritize semantic relevance.\n"
    "4. Prefer entries with function value 'CAA-Components' over 'CATIA-Commands'.\n"
    "5. Return only the exact component name with coordinates if applicable.\n"
    "6. Do not add any explanations, comments, or extra text.\n"
    "7. If no perfect match exists, return the closest available one.\n"
    "8. If no usable match exists, and the user input appears to be a question (e.g., ends with '?'), respond with a comprehensive and technical answer based only on the JSON context.\n"
    "9. If the input is not a question and no relevant match can be found, respond with helpful suggestions based on provided json documents, for example:\n"
        "Unable to find a relevant information or CATIA component.\n"
        "Please try rephrasing your request, question, or CATIA instruction more clearly.\n"
        "You could try asking more specific questions such as:\n"
        "- 'How do I create a 3D point at coordinates 5.0;6.0;7.0?'\n"
        "- 'What command is used to generate a line between two points?'\n"
        "- 'Is there a component for inserting a CATPart into an assembly?'\n"
        "- 'Which function creates a sketch-based feature?'\n"
        "\n"
    "Your output must strictly follow these rules without deviation."
)
# --------------------------------------------------------------------------------------------------
# instruction
 
# --------------------------------------------------------------------------------------------------
# create_prompt
# --------------------------------------------------------------------------------------------------
def create_prompt(context, question):
    # Regular expression to match coordinates like "40.9;89.0;34.0"
    coord_pattern = re.compile(r"([-+]?[0-9]*\.?[0-9]+);([-+]?[0-9]*\.?[0-9]+);([-+]?[0-9]*\.?[0-9]+)")
    
    # Find all matches for coordinates in the question
    matches = coord_pattern.findall(question)
    coordinates = ""

    if matches:
        # If we have multiple sets of coordinates, format them into the desired form
        coordinates = ";".join([f"{x};{y};{z}" for x, y, z in matches])
    
    # Build the base prompt to search for components in the context
    prompt = f"""
    You are an AI agent with extensive expertise in the interpretation
    of CAA components in JSON format. Your task is to determine the appropriate CAA component based on a user’s natural language input
    (e.g., 'Create a 3D point' or in other language, e.g. German, French, etc.) by searching the provided documentation or JSON structure, and return the exact name of the command.

    Rules:
    1. Analyze the meaning of the user's input.
    2. Search the JSON structure (provided below as 'context') for a component that matches the requested action in any natural language.
       Look for descriptions, keywords, function names, or command references such as "name", "description", "command", "function".
    3. Prefer entries with function value 'CAA-Components' over 'CATIA-Commands'.
    4. Return only the exact name of the component with coordinates if available, such as "UTLGo3DPoint;8.9;8.0;4.32", "UTLGo3DLine;1.23;8.98;2.34;8.9;8.0;4.32", or "UTLGoCATPart".
    5. No additional explanations, comments, or text.
    6. If no highly suitable match is found in the context, return the closest available match, even if it is only loosely related.
    7. If no usable match exists at all, and the user's input appears to be a question (e.g. it ends with a "?"), interpret it as such and provide a comprehensive answer based on the information available in the documents. 
    8. If the input is not a question and no relevant match can be found, respond with helpful suggestions based on provided json documents, for example:\n"
        "Unable to find a relevant information or CATIA component.\n"
        "Please try rephrasing your request, question, or CATIA instruction more clearly.\n"
        "You could try asking more specific questions such as:\n"
        "- 'How do I create a 3D point at coordinates 5.0;6.0;7.0?'\n"
        "- 'What command is used to generate a line between two points?'\n"
        "- 'Is there a component for inserting a CATPart into an assembly?'\n"
        "- 'Which function creates a sketch-based feature?'\n"
        "\n"
    "Your output must strictly follow these rules without deviation."
    Examples:

    User request:
    "Create a 3D point in 5.0;9.7;1.89" or "new point in 8;8;0.8"

    JSON Context:
    {{
        "components": [
            {{
                "name": "UTLGo3DPoint",
                "description": "Creates a point in 3D space."
            }},
            {{
                "name": "UTLGo3DLine",
                "description": "Connects two points with a line."
            }}
        ]
    }}

    Output:
    UTLGo3DPoint;5.0;9.7;1.89
    --------------------

    User request:
    "Create a 3D Line from 5.0;9.7;1.89 to 6.12;8.9;0.12"

    JSON Context:
    {{
        "components": [
            {{
                "name": "UTLGo3DPoint",
                "description": "Creates a point in 3D space."
            }},
            {{
                "name": "UTLGo3DLine",
                "description": "Connects two points with a line."
            }}
        ]
    }}

    Output:
    UTLGo3DLine;5.0;9.7;1.89;6.12;8.9;0.12
    --------------------

    User request:
    {question}

    JSON Context:
    {context}

    Output:
    """

    # If coordinates are found, append them to the output in the required format
    prompt += f";{coordinates}"
    
    return prompt
# --------------------------------------------------------------------------------------------------
# create_prompt
 
# --------------------------------------------------------------------------------------------------
# ask_openai2
# --------------------------------------------------------------------------------------------------
def ask_openai2(context, question): 
    # Inject context and question into the instruction
    # formatted_instruction = instruction.replace("{{context}}", context).replace("{{question}}", question)

    # Generate the user prompt (with examples and formatting)
    prompt = create_prompt(context, question)  
    
    # Make the API request
    response = client_ai.completions.create(
        model=llm_model,
        prompt=prompt,
        max_tokens=1024,
        temperature=0.28,
        top_p=0.95,
        n=1,
        echo=True,
        stream=False
    )
    return response.choices[0].text.strip()
# --------------------------------------------------------------------------------------------------
# ask_openai2

# --------------------------------------------------------------------------------------------------
# ask_openai
# --------------------------------------------------------------------------------------------------
def ask_openai(context, question): 
    # Inject context and question into the instruction
    formatted_instruction = instruction.replace("{{context}}", context)\
                                       .replace("{{question}}", question)

    # Generate the user prompt (with examples and formatting)
    prompt = create_prompt(context, question)

    # Call the OpenAI API
    response = client_ai.chat.completions.create(
        model=llm_model,
        messages=[
            {"role": "system", "content": formatted_instruction},
            {"role": "user", "content": prompt}
        ],
        temperature=0.28,
        max_tokens=MAX_TOKENS
    )
    return response.choices[0].message.content.strip()
# --------------------------------------------------------------------------------------------------
# ask_openai

# --------------------------------------------------------------------------------------------------
# ask_openai1
# --------------------------------------------------------------------------------------------------
def ask_openai1( context, question):
    client = OpenAI()  # Uses OPENAI_API_KEY from environment variables
    prompt = create_prompt( context, question)
    
    response = client.chat.completions.create(
        model=llm_model,
        messages=[
            {"role": "system", "content": instruction},
            {"role": "user", "content": prompt}
        ],
        temperature=0.3,
        max_tokens=MAX_TOKENS
    )
    return response.choices[0].message.content.strip()
# --------------------------------------------------------------------------------------------------
# ask_openai1
 
# --------------------------------------------------------------------------------------------------
# askAI
# --------------------------------------------------------------------------------------------------
def askAI(query, person, i_CollectionName):    
    # global collection_name
    # collection_name = i_CollectionName
    
    # Retrieve relevant files based on the query
    relevant_files = retrieve_relevant_files(query)
        
    print( "collection_name:", collection_name)

    # Combine relevant files' content
    # combined_content = "\n".join(  [str(item) for sublist in relevant_files for item in sublist]
    combined_content = "\n".join([str(item) for item in relevant_files]) 

    # Check token length of the combined content
    total_tokens = get_token_length(combined_content) 
    if total_tokens > MAX_TOKENS:
        combined_content = truncate_input(combined_content, max_tokens=MAX_TOKENS - get_token_length(query))

    # If the content is still too long, split into chunks
    chunks = split_into_chunks(combined_content, chunk_size=3500)
     
    final_response = ask_openai(combined_content, query)
    return final_response
# --------------------------------------------------------------------------------------------------
# askAI
 
# --------------------------------------------------------------------------------------------------
# storeDB
# --------------------------------------------------------------------------------------------------
def storeDB(text, doc_id, metadata=None): 
    # Generate the query embedding vector
    embedding_model = OpenAIEmbeddings(model=text_embedding_model )
    query_embedding = embedding_model.embed_query(text) 
   
    # Check if the text has already been added to ChromaDB    
    collection = client_db.get_or_create_collection( collection_name )           

  

    # Insert the content and embeddings into ChromaDB
    collection.add(
        documents=[text],
        embeddings=[query_embedding],
        metadatas=[None],
        ids=[str(doc_id)],  # Unique ID for the content
    )        
    print(f"Document ID '{doc_id}' added to ChromaDB collection '{collection_name}'")    
# --------------------------------------------------------------------------------------------------
# storeDB

# --------------------------------------------------------------------------------------------------
# storeDB
# --------------------------------------------------------------------------------------------------
def clearDB( doc_id ): 
    # global collection_name
    # collection_name = doc_id
    try:
        print(client_db.list_collections())
        
        collection = client_db.get_or_create_collection(collection_name)
        
        # Fetch all documents to get their IDs
        all_docs = collection.get()
        if 'ids' in all_docs and all_docs['ids']:
            # collection.delete(ids=all_docs['ids'])
            print(f"Collection NOT cleaned: '{collection_name}' ({len(all_docs['ids'])} documents removed)")
        else:
            print(f"Collection '{collection_name}' is already empty.")
            
    except Exception as e:
        print(f"Error clearing collection '{collection_name}': {e}")
# --------------------------------------------------------------------------------------------------
# clearDB

# --------------------------------------------------------------------------------------------------
# storeDB_bulk1
# --------------------------------------------------------------------------------------------------
def storeDB_bulk1(texts, doc_ids, metadatas):
    """
    Store multiple documents in ChromaDB at once.

    Parameters:
        texts (list of str): The documents to embed and store.
        doc_ids (list of str): Unique IDs for each document.
        metadatas (list of dict, optional): Optional metadata for each document.
    """ 
    
    # Initialize the embedding model 
    embedding_model = OpenAIEmbeddings(model=text_embedding_model)

    # Generate embeddings for all documents
    embeddings = embedding_model.embed_documents(texts)

    # Create or get the collection
    collection = client_db.get_or_create_collection(collection_name)

  
    # Add all documents at once
    collection.add(
        documents=texts,
        embeddings=embeddings,
        metadatas=metadatas,
        ids=doc_ids,
    )

    print(f"{len(texts)} documents added to ChromaDB.")
# --------------------------------------------------------------------------------------------------
# storeDB_bulk1

# --------------------------------------------------------------------------------------------------
# storeDB_bulk
# --------------------------------------------------------------------------------------------------
def storeDB_bulk(texts, doc_ids, metadatas):
    """
    Store multiple documents in ChromaDB at once.

    Parameters:
        texts (list of str): The documents to embed and store.
        doc_ids (list of str): Unique IDs for each document.
        metadatas (list of dict, optional): Optional metadata for each document.
    """     
    def is_valid_utf8(s):
        try:
            if isinstance(s, bytes):
                s.decode('utf-8')
            else:
                s.encode('utf-8')
            return True
        except UnicodeError:
            return False

    # --- Validate inputs ---
    for i, metadata in enumerate(metadatas or []):
        for key, value in metadata.items():
            if isinstance(value, str) and not is_valid_utf8(value):
                print(f"metadata string value at index {i}, key '{key}' is not valid UTF-8")
                
    for i, doc_id in enumerate(doc_ids):
        if not is_valid_utf8(doc_id):
            print(f"doc_id at index {i} is not valid UTF-8: '{doc_id}'")
                       
    for i, text in enumerate(texts):
        if not is_valid_utf8(text):
            print(f"text at index {i} is not valid UTF-8: '{text}'")
    
    # --- Proceed if all strings are valid ---
    embedding_model = OpenAIEmbeddings(model=text_embedding_model)
    embeddings = embedding_model.embed_documents(texts)
    collection = client_db.get_or_create_collection(collection_name)

    collection.add(
        documents=texts,
        embeddings=embeddings,
        metadatas=metadatas,
        ids=doc_ids,
    )

    print(f"{len(texts)} documents added to ChromaDB.")
# --------------------------------------------------------------------------------------------------
# storeDB_bulk

# for test purpose only
# db_collection = "d756002f_5d0_688b7022_4c"
# query = "what is uuid of CATPrtCont?"
# aiAnswer = askAI(query, "Donald Trump", db_collection)
# print("AI says:\n", aiAnswer)

#-----------------------------------------------------------------------------------
# encode_img_to_b64
#-----------------------------------------------------------------------------------
def encode_img_to_b64(img_path ):  
    img2 = Image.open(img_path)
    buf2 = io.BytesIO()
    img2.save(buf2, format="PNG")
    img2_b64 = base64.b64encode(buf2.getvalue()).decode("utf-8")       
    return img2_b64
#-----------------------------------------------------------------------------------
# encode_img_to_b64

#-----------------------------------------------------------------------------------
# describe_images
#-----------------------------------------------------------------------------------
def describe_images(img_paths, prompt: str):
    """
    Send multiple PNG projections to GPT‑4o and ask if they belong to the
    same 3D CAD model.
    """
    response = client_ai.chat.completions.create(
        model="gpt-5-chat-latest", # "gpt-4o",   # <- must be a vision‑capable model
        # model="gpt-5-mini", # "gpt-4o",   # <- must be a vision‑capable model
        messages=[
            {
                "role": "system",
                "content": (
                    "You are an expert in CAD and geometry and can analyze images of 3D CATIA models and their 2D drawings." 
                ),
            },
            {
                "role": "user",
                "content": [
                    {
                        "type": "text",
                        "text": prompt
                    },
                    *[
                        {
                            "type": "image_url",
                            "image_url": { "url": f"data:image/png;base64,{encode_img_to_b64(path)}"},
                        }
                        for path in img_paths
                    ],
                ],
            },
        ],
    )
    return response.choices[0].message.content
#-----------------------------------------------------------------------------------
# describe_images 

# ------------------------------------------------------------------------
# Apply to all images in a directory
# ------------------------------------------------------------------------
def process_directory(directory_path, prompt: str): 
    # collect all PNG files
    image_paths = [
        os.path.join(directory_path, fn)
        for fn in os.listdir(directory_path)
        if fn.lower().endswith(".png")
    ]

    # make sure you have at least 2 images
    if len(image_paths) < 1:
        print(f"⚠️ Not enough images in {directory_path}")
        return

    # call the compare function
    result = describe_images(image_paths, prompt)
    return result 
# ------------------------------------------------------------------------
# Apply to all images in a directory

# --------------------------------------------------------------------------------------------------
# askAIAboutPart
# --------------------------------------------------------------------------------------------------
def askAIAboutPart( user_question ):    
    # f‑String: {user_question} wird korrekt eingesetzt
    prompt = (
        f"Welches dieser Bauteile wird benötigt, um die folgende Anfrage zu erfüllen? "
        f"Gib nur die Nummer des Bauteils zurück. "
        f"Anfrage: {user_question}"
    )
    
    img_path = r"C:\tmp\pngs" 

    # Assume this function returns a text description
    description = process_directory(img_path, prompt)
    print(description)
    return description 
# --------------------------------------------------------------------------------------------------
# askAIAboutPart