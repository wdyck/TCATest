# import Catia
import win32gui
import pythoncom
import win32com.client
import time
import io
from contextlib import redirect_stdout
import difflib
import os                         # ← add this line
from win32com.client.dynamic import CDispatch as CatiaApp
import shutil
import sys
from pywinauto import Application, Desktop
import re
import string
import cv2
import numpy as np 
import base64 
import pyautogui
from openai import OpenAI  
from PIL import Image
import requests
import yfinance as yf
from datetime import datetime, timedelta
import tkinter as tk
from tkinter import scrolledtext
import json, os
import pytesseract
from segment_anything import sam_model_registry, SamAutomaticMaskGenerator
import torch, cv2


# Initialize OpenAI client (uses environment variable: OPENAI_API_KEY)
client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
 

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
    response = client.chat.completions.create(
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
 
#-----------------------------------------------------------------------------------
# __main__
#-----------------------------------------------------------------------------------
if __name__ == '__main__1':  
    prompt = (
      "Based on the following images of a car component captured from multiple viewpoints, "
      "provide an objective and detailed description of what is visibly observable. "
      "Describe only features that can be clearly seen in the images — such as shapes, contours, "
      "openings, mounting points, or overall position relative to recognizable parts of the vehicle. "
      "Do not infer hidden structures, materials, or functions that are not explicitly visible. "
      "The description should begin, for example, as: "
      "‘This component appears to be a car bumper and shows the following visible characteristics "
      "and position within the vehicle: …’"
    )

    
    img_path = r"C:\tmp\left_door" 

    # Assume this function returns a text description
    description = process_directory(img_path, prompt)

    # Prepare file path
    desc_file = os.path.join(img_path, "description.txt")

    # Write the description text to a file
    with open(desc_file, "w", encoding="utf-8") as f:
        f.write(description)

    print(f"Description saved to: {desc_file}")
#-----------------------------------------------------------------------------------
# __main__ 

# ------------------------------------------------------------
# build the multimodal prompt
# ------------------------------------------------------------
PROMPT_TEMPLATE = """
  You are a CAD‑image analysis assistant.

  The image shows a 3D CAD render of a car component
  (e.g. door, bumper, fender, roof).  Each face has a
  numeric label (1, 2, 3, … n).

  Task:
  Identify all face numbers that visually belong to the
  **{target_area}** of the part.

  Rules:
  1. Inspect the labeled faces carefully.
  2. List every visible face number on or adjacent to
     the {target_area}.
  3. Output valid JSON only:

  ```json
  {{
    "target_area": "{target_area}",
    "face_numbers": [ list of integers ],
    "reasoning": "short explanation"
  }}"""

#-----------------------------------------------------------------------------------
# describe_image
#-----------------------------------------------------------------------------------
def describe_image(img_path: str, prompt: str): 
  model_name = "gpt-5-chat-latest"
  response = client.chat.completions.create(
      model=model_name,
      messages=[
          {
              "role": "system",
              "content": "You are an expert in CAD and geometry analysis.",
          },
          {
              "role": "user",
              "content": [
                  {
                      "type": "text",
                      "text": prompt,
                  },
                  {
                      "type": "image_url",
                      "image_url": { "url": f"data:image/png;base64,{encode_img_to_b64(img_path)}"},
                  },
              ],
          },
      ],
  )
  return response.choices[0].message.content
#-----------------------------------------------------------------------------------
# describe_image
 
# ------------------------------------------------------------------------------
# describe_image_ocr
# ------------------------------------------------------------------------------
def describe_image_ocr(ocr_text: str):
    """
    Send OCR-extracted text to a language model and ask it
    to group face numbers into functional regions.
    """
    model_name = "gpt-5-chat-latest"
    ocr_prompt = f"""
    You are a CAD assistant.
    The following text was extracted from a screenshot of a labeled CAD model.

    Text OCR output:

    {ocr_text}


    Task:
    - Identify all numbers that correspond to visible faces.
    - Group them into functional regions such as door handle, hinge area, window frame, etc.
    Return the result as JSON:
    {{
      "regions": [
        {{"name": "door handle", "face_numbers": [ ... ]}},
        {{"name": "hinge", "face_numbers": [ ... ]}}
      ]
    }}
    """

    response = client.chat.completions.create(
        model=model_name,
        messages=[
            {
                "role": "system",
                "content": "You are an expert in CAD and geometry analysis.",
            },
            {
                "role": "user",
                "content": ocr_prompt,
            },
        ],
    )
    return response.choices[0].message.content
# ------------------------------------------------------------------------------
# describe_image_ocr

#-----------------------------------------------------------------------------------
# describe_one_image
#-----------------------------------------------------------------------------------
def describe_one_image(img_path: str, prompt: str): 
  model_name = "gpt-5-chat-latest"
  response = client.chat.completions.create(
      model=model_name,
      messages=[
          {
              "role": "system",
              "content": "You are an expert in CAD and geometry analysis.",
          },
          {
              "role": "user",
              "content": [
                  {
                      "type": "text",
                      "text": prompt,
                  },
                  {
                      "type": "image_url",
                      "image_url": { "url": f"data:image/png;base64,{encode_img_to_b64(img_path)}"},
                  },
              ],
          },
      ],
  )
  return response.choices[0].message.content
#-----------------------------------------------------------------------------------
# describe_one_image


#-----------------------------------------------------------------------------------
# __main__
#-----------------------------------------------------------------------------------
if __name__ == '__main__2': 
    img_path = r"C:\tmp\left_door"
    img_file = os.path.join(img_path, "faa0001_a988_69985739_537.png")

    prompt = "which numbers are displayed?"
    image_description = describe_one_image(img_file, prompt)
    print( image_description )     

    # 3️⃣  Save result
    desc_file = os.path.join(img_path, "description.txt")
    with open(desc_file, "w", encoding="utf-8") as f:
        f.write(image_description)

    print(f"Description saved to: {desc_file}")
#-----------------------------------------------------------------------------------
# __main__ 

#-----------------------------------------------------------------------------------
# find_door_handle_area
#-----------------------------------------------------------------------------------
def find_door_handle_area(ocr_text, img_path):
    # Bild zu base64‑String (optional, wenn Model Vision versteht) 

    prompt = f"""
    You are an assistant for analyzing labeled CAD images of car doors.
    Each red number corresponds to a surface region.

    OCR text extracted from the image:

    {ocr_text}

    Task:
    Identify which of these numbered regions belong to the **door‑handle area** of the vehicle door.
    Return strictly JSON:
    {{
      "door_handle_faces": [list of numbers],
      "reasoning": "short explanation"
    }}
    """
    model_name = "gpt-5-chat-latest"

    response = client.chat.completions.create(
        model=model_name,
        messages=[
            {
                "role": "system",
                "content": "You are an expert in CAD and geometry analysis.",
            },
            {
                "role": "user",
                "content": [
                    {
                        "type": "text",
                        "text": prompt,
                    },
                    {
                        "type": "image_url",
                        "image_url": { "url": f"data:image/png;base64,{encode_img_to_b64(img_path)}"},
                    },
                ],
            },
        ],
    )
    return response.choices[0].message.content
#-----------------------------------------------------------------------------------
# find_door_handle_area

# ------------------------------------------------------------
# main get_door_handle_bbox
# ------------------------------------------------------------
def get_door_handle_bbox(img_path: str, target_area="door handle"): 

    prompt = f"""
      You are a vision expert for CAD geometry.
      Look at this labeled car‑door image and estimate where the
      **{target_area}** is located in pixel coordinates.

      Return a JSON object with the approximate bounding box and
      short reasoning.  Coordinates must fit the image size.

      Example:
      {{
        "target_area": "{target_area}",
        "bbox": [x1, y1, x2, y2],
        "reasoning": "why this area corresponds to the door handle"
      }}"""

    response = client.chat.completions.create(
        model= "gpt-5-chat-latest",                                   # vision‑capable model
        messages=[
            {"role": "system", "content": "You analyze CAD images and estimate regions of interest."},
            {"role": "user",
             "content": [
                    {
                        "type": "text",
                        "text": prompt,
                    },
                    {
                        "type": "image_url",
                        "image_url": { "url": f"data:image/png;base64,{encode_img_to_b64(img_path)}"},
                    },
             ]},
        ],
        max_tokens=500,
    )

    message = response.choices[0].message.content
    try:
        # try to parse JSON part if present
        start = message.find("{")
        end   = message.rfind("}") + 1
        bbox_json = json.loads(message[start:end])
        return bbox_json
    except Exception:
        return {"raw_output": message}
# ------------------------------------------------------------
# main get_door_handle_bbox

# ------------------------------------------------------------
# run from CLI
# ------------------------------------------------------------
if __name__ == "__main__CLI":

    img_path = r"C:\tmp\left_door"
    img_file = os.path.join(img_path, "door.png") 
    result = get_door_handle_bbox(img_file)

    bbox = result.get("bbox")
    if bbox and len(bbox) == 4:
      img = cv2.imread(img_file)
      x1, y1, x2, y2 = map(int, bbox)
      cv2.rectangle(img, (x1, y1), (x2, y2), (0, 255, 0), 3)
      cv2.imwrite(r"C:\tmp\left_door\door_with_bbox.png", img)
      print("Saved overlay → door_with_bbox.png")

    print("\n--- Model Result ---")
    print(json.dumps(result, indent=2))
    
    #   Save result
    # desc_file = os.path.join(img_path, "description.txt")
    # with open(desc_file, "w", encoding="utf-8") as f:
    #    f.write(result)
# ------------------------------------------------------------
# run from CLI


#-----------------------------------------------------------------------------------
# __main__
#-----------------------------------------------------------------------------------
if __name__ == '__main__masks':
  
  img_path = r"C:\tmp\left_door"
  img_file = os.path.join(img_path, "door.png") 

  sam = sam_model_registry["vit_h"](checkpoint=r"C:\EXAMPLES\AI\PTH\sam_vit_h_4b8939.pth")
  sam.to(device="cuda" if torch.cuda.is_available() else "cpu")

  img = cv2.imread(img_file)
  img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

  mask_generator = SamAutomaticMaskGenerator(sam, points_per_side=32)
  masks = mask_generator.generate(img_rgb)

  print(f"{len(masks)} masks found")

  # write each mask to a separate file
  for i, m in enumerate(masks):
      mask = (m["segmentation"].astype("uint8")) * 255
      area = m.get("area", 0)
      out_file = os.path.join(img_path, f"mask_{i:03d}_area_{area}.png")
      cv2.imwrite(out_file, mask)
      print("saved:", out_file)




#-----------------------------------------------------------------------------------
# __main__
#-----------------------------------------------------------------------------------
if __name__ == '__main__ocr': 
    img_path = r"C:\tmp\left_door"
    img_file = os.path.join(img_path, "door.png") 

    # OCR extraction (use image_to_string for text, not image_to_data)     
    img = cv2.imread(img_file)

    # ---------------------------------------------
    #  Rote Bereiche herausfiltern (HSV)
    # ---------------------------------------------
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    lower1 = np.array([0, 80, 80])
    upper1 = np.array([10, 255, 255])
    lower2 = np.array([160, 80, 80])
    upper2 = np.array([180, 255, 255])
    mask = cv2.bitwise_or(cv2.inRange(hsv, lower1, upper1),
                          cv2.inRange(hsv, lower2, upper2))
    red_only = cv2.bitwise_and(img, img, mask=mask)

    # ---------------------------------------------
    #  Optional: In Graustufen umwandeln und kontrastverbessern
    # ---------------------------------------------
    gray = cv2.cvtColor(red_only, cv2.COLOR_BGR2GRAY)
    gray = cv2.threshold(gray, 0, 255,
                         cv2.THRESH_BINARY + cv2.THRESH_OTSU)[1]

    # ---------------------------------------------
    # OCR auf rot gefiltertes Bild
    # ---------------------------------------------
    text = pytesseract.image_to_string(gray, config="--psm 6 digits")
    print("Erkannte Ziffern:")
    print(text)
        
    result = find_door_handle_area(text, img_file)
    print(result)

    #   Save result
    desc_file = os.path.join(img_path, "description.txt")
    with open(desc_file, "w", encoding="utf-8") as f:
        f.write(result)

    print(f"Description saved to: {desc_file}")
#-----------------------------------------------------------------------------------
# __main__ 

#-----------------------------------------------------------------------------------
# __main__
#-----------------------------------------------------------------------------------
if __name__ == '__main__pngs':  
    user_question = "Finde und vermesse den Kotflügel mit der Öffnung für die Ladeklappe"

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
     
#-----------------------------------------------------------------------------------
# __main__

#-----------------------------------------------------------------------------------
#  find_best_match
#-----------------------------------------------------------------------------------
def find_best_match():
    """
    This function constructs a precise analysis prompt and processes
    images from 'C:\\tmp\\doors' to determine which red hole best
    matches the single green hole. The match criterion primarily
    considers hole position, then size and shape.
    """

    #--- 1. Construct the detailed prompt ------------------------------------------
    prompt_text = (
        f"Identify which red‑marked hole most closely corresponds to the green‑marked reference hole. "
        f"Base the comparison on the hole’s position, size, shape, geometric characteristics, "
        f"and spatial relationships to neighboring holes within the part. "
        f"The hole’s position is the primary matching criterion. "
        f"If the positions are approximately the same, give higher priority to the hole’s shape and size rather than its exact location. "
        f"Return only the letter of the matching hole."
    )

    #--- 2. Define directory to process --------------------------------------------
    img_path = r"C:\tmp\doors"

    #--- 3. Process directory with given prompt ------------------------------------
    try:
        # process_directory should analyze the image set according to the prompt
        description = process_directory(img_path, prompt_text)
    except Exception as exc:
        print(f"ERROR : Processing of directory failed ({exc})")
        return

    #--- 4. Output formatted result -------------------------------------------------
    print("----------------------------------------------------------------")
    print("Best‑Match Ergebnis:")
    print(description)
    print("----------------------------------------------------------------\n")
 
#-----------------------------------------------------------------------------------
# find_best_match

#-----------------------------------------------------------------------------------
# copy_green_images
#-----------------------------------------------------------------------------------
def copy_green_images():
    """
    Entspricht der CAA‑Logik:
        CATOpenDirectory("C:\\tmp\\doors\\green")
        CATFCopy(source, destination, 0)
    Kopiert nacheinander jedes .png‑Bild aus dem Verzeichnis
    C:\\tmp\\doors\\green nach C:\\tmp\\doors unter dem Namen green.png.
    Die Zieldatei wird bei jedem Kopiervorgang überschrieben.
    """

    src_dir = r"C:\tmp\doors\green"
    dst_dir = r"C:\tmp\doors"
    dst_file = os.path.join(dst_dir, "green.png")

    # Prüfen, ob Quellverzeichnis vorhanden ist (analog CATOpenDirectory)
    if not os.path.isdir(src_dir):
        print(f"ERROR : Can't open directory {src_dir}")
        return

    # Sicherstellen, dass Zielverzeichnis existiert
    if not os.path.isdir(dst_dir):
        os.makedirs(dst_dir)

    # Durch alle .png‑Dateien iterieren (wie listOfV4Files)
    png_files = [f for f in os.listdir(src_dir) if f.lower().endswith(".png")]

    for file_name in png_files:
        src_file = os.path.join(src_dir, file_name)
        print(f"Copying {src_file} → {dst_file}")
        shutil.copy(src_file, dst_file)   # entspricht CATFCopy
        find_best_match()

    print("All .png files processed.")
#-----------------------------------------------------------------------------------
# copy_green_images

#-----------------------------------------------------------------------------------
# __main__
#-----------------------------------------------------------------------------------
if __name__ == '__main__ASSIGN':   
  copy_green_images()      
#-----------------------------------------------------------------------------------
# __main__


#-----------------------------------------------------------------------------------
# __main__
#-----------------------------------------------------------------------------------
if __name__ == '__main__':   

  # prompt = "list all red holes by their letters"
  # prompt = "list best matches of red holes for the green hole '11' as table including holes letters and match confidence values"
  prompt = "which red hole has the best match for the green hole '11'? Return only the red letter of the hole"
  img_path = r"C:\tmp\doors\old_new.png"
  description = describe_image(img_path, prompt)
  # description = process_directory(img_path, prompt)
  print( description)
#-----------------------------------------------------------------------------------
# __main__ 