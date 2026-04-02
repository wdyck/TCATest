import Catia
import win32gui
import pythoncom
import win32com.client
import time
import io
import sys
from contextlib import redirect_stdout
import difflib
import os                         # ← add this line
from win32com.client.dynamic import CDispatch as CatiaApp
import shutil
import sys
from pywinauto import Application, Desktop
import re
import string

import base64 
import pyautogui
from openai import OpenAI  
from PIL import Image

# Initialize OpenAI client (uses environment variable: OPENAI_API_KEY)
client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))

cmd_mta_wkb_path = r"C:\EXAMPLES\MTA\TEST\CMD_MTAWorkbench.txt"
out_path = r"C:\EXAMPLES\MTA\TEST\refs\\"
log_path = r"C:\EXAMPLES\MTA\TEST\logs\\"
max_wait=15.0



# -----------------------------------------------------------------------------------
# parse_line_to_set
# -----------------------------------------------------------------------------------
def parse_line_to_set(line: str, replace_unprintable: bool = False) -> set[str]:
    """
    Parse a line like "['Dialog', 'MTA-PMP.1', 'MTA-PMP.1Dialog']"
    into a set of clean strings, removing or replacing strange characters.

    Args:
        line (str): The raw line text.
        replace_unprintable (bool): 
            False -> remove non‑ASCII chars,
            True  -> replace them with '?'.
    Returns:
        set[str]: cleaned tokens
    """
    # --- basic cleanup of punctuation and wrappers ---
    cleaned = (
        line.strip()
            .replace("[", "")
            .replace("]", "")
            .replace("{", "")
            .replace("}", "")
            .replace("'", "")
            .replace('"', "")
            .replace('| ', "")
    )

    tokens = [t.strip() for t in cleaned.split(",") if t.strip()]

    safe_tokens = []
    for t in tokens:
        if replace_unprintable:
            # replace every non‑printable / non‑ASCII char with '?'
            safe = re.sub(r"[^\x20-\x7E]", "?", t)
        else:
            # remove them entirely
            safe = "".join(ch for ch in t if ch in string.printable)

        if safe:
            safe_tokens.append(safe)

    return set(safe_tokens)
# -----------------------------------------------------------------------------------
# parse_line_to_set


#-----------------------------------------------------------------------------------
# delete_all_contents
#-----------------------------------------------------------------------------------
def delete_all_contents(directory: str) -> None:
    for name in os.listdir(directory):
        path = os.path.join(directory, name)
        try:
            if os.path.isfile(path) or os.path.islink(path):
                os.remove(path)
            elif os.path.isdir(path):
                shutil.rmtree(path)
        except Exception as e:
            print(f"⚠️ Failed to delete {path}: {e}")
#-----------------------------------------------------------------------------------
# delete_all_contents

#-----------------------------------------------------------------------------------
# get_cmd_dlg
#-----------------------------------------------------------------------------------
def get_cmd_dlg(cmd_name: str, catapp: CatiaApp): 
   
    # connect to running CATIA session
    app = Application().connect(title_re="CATIA V5.*")

    before_windows = [w for w in app.windows()
                      if w.is_visible()
                      and w.children(title_re="(?i).*Cancel.*")]

    for title in before_windows:
        print("old: ", title)

    file_path = os.path.join(out_path, f"{cmd_name}.txt") 
    catapp.StartCommand(cmd_name)
    
    # wait until dialog appears
    time.sleep(2)

    # ----- poll for new window(s) -----
    start_time = time.time()
    after_windows_all = []

    while time.time() - start_time < max_wait:
        all_win = [
            w for w in app.windows()
            if w.is_visible() and w.children(title_re="(?i).*Cancel.*")
        ]
        after_windows_all = [w for w in all_win if w.handle not in before_windows]
        if after_windows_all:
            break 

    # keep only new ones (not seen before)
    after_windows = [w for w in after_windows_all if w.handle not in before_windows]

    for title in after_windows:
        print("new ", title)    
        
    if after_windows:
        dlg = after_windows[0]
    else: 
        print("No new windows detected") 
        return None

    return dlg 
#-----------------------------------------------------------------------------------
# get_cmd_dlg

# -----------------------------------------------------------------------------------
# close_dialog
# -----------------------------------------------------------------------------------
def close_dialog(dlg) -> bool:
    try:
        buttons = dlg.descendants(title="Close")
        for b in buttons:
            title = b.window_text().strip()
            cls   = b.friendly_class_name()
            print(f"  - {title or '(no text)'}   [{cls}]")
            b.click() 
        dlg.close()
        return True
    except Exception as e:
        print(f"⚠ Could not close dialog '{dlg.window_text()}': {e}")
        return False
# -----------------------------------------------------------------------------------

#-----------------------------------------------------------------------------------
# cmd_dump1
#-----------------------------------------------------------------------------------
def cmd_dump1(cmd_name: str, catapp: CatiaApp, app: Application) -> None: 
  
    file_path = os.path.join(out_path, f"{cmd_name}.txt")    

    dlg = get_cmd_dlg(cmd_name, catapp) 
     
    dlg_spec = app.window(handle=dlg.handle)

    with open(file_path, "w", encoding="utf-8") as f:
      old_stdout = sys.stdout        # save current stdout
      sys.stdout = f                 # redirect to file
      dlg_spec.print_control_identifiers(depth=2)
      sys.stdout = old_stdout        # restore console output

    print(f"{cmd_name} identifiers written to {file_path}")

    # try to click close button
    if close_dialog(dlg):
      return;

    # cancel dlg
    try:
        dlg.type_keys("{ESC}")  
    except Exception:
        dlg.type_keys("{ENTER}")   # fallback 

    return
#-----------------------------------------------------------------------------------
# cmd_dump1

#-----------------------------------------------------------------------------------
# dump_controls
#-----------------------------------------------------------------------------------
def dump_controls(ctrl, level=0, lines=None):
    if level > 1:
      return lines

    if lines is None:
        lines = []
    indent = "  " * level
    name = ctrl.window_text().strip() or ctrl.friendly_class_name()
    clean = name.replace("\n", "")
    lines.append(f"{indent}{ctrl.friendly_class_name()} - '{clean}'")
    for child in ctrl.children():
        dump_controls(child, level+1, lines)
    return lines
  
#-----------------------------------------------------------------------------------
# get_dlg_content
#-----------------------------------------------------------------------------------
def get_dlg_content(dlg_spec) -> str:
    return "\n".join(dump_controls(dlg_spec))
#-----------------------------------------------------------------------------------
# get_dlg_content

#-----------------------------------------------------------------------------------
# cmd_dump  --  deduplicated identifiers
#-----------------------------------------------------------------------------------
def cmd_dump(cmd_name: str, catapp: CatiaApp, app: Application) -> None:
    """Dump CATIA command dialog identifiers (each only once)."""
    dlg = get_cmd_dlg(cmd_name, catapp)
    dlg_spec = app.window(handle=dlg.handle)

    file_path = os.path.join(out_path, f"{cmd_name}.txt")

    # collect unique lines before writing
    dlgContent = get_dlg_content(dlg_spec) 

    # print(dlgContent)

    with open(file_path, "w", encoding="utf-8") as f:
        f.write(dlgContent)

    print(f"{cmd_name} identifiers written once to {file_path}")

    # try to click close button
    if close_dialog(dlg):
      return;

    # Close the dialog
    try:
        dlg.type_keys("{ESC}")
    except Exception:
        dlg.type_keys("{ENTER}")

    return;
#-----------------------------------------------------------------------------------
# cmd_dump  --  deduplicated identifiers

#-----------------------------------------------------------------------------------
# cmd_comp
#-----------------------------------------------------------------------------------
def cmd_comp(cmd_name: str, catapp: CatiaApp, app: Application) -> None: 

    file_path = os.path.join(out_path, f"{cmd_name}.txt") 
    logs_path = os.path.join(log_path, f"{cmd_name}.log") 
    dlg = get_cmd_dlg(cmd_name, catapp)     
    dlg_spec = app.window(handle=dlg.handle)

    # collect unique lines before writing
    dlgContent = get_dlg_content(dlg_spec) 

    # try to click close button
    if not close_dialog(dlg):
        # cancel dlg
        try:
            dlg.type_keys("{ESC}")  
        except Exception:
            dlg.type_keys("{ENTER}")   # fallback 

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            file_text = f.read()
    except FileNotFoundError:
        file_text = ""        # or None
        print(f"⚠ Skipping missing file: {file_path}")
        with open(logs_path, "w", encoding="utf-8") as f, redirect_stdout(f):
          print("missing reference file: ", file_path)
          return

    # Normalize and split
    lines_current = [line.strip() for line in dlgContent.splitlines() if line.strip()]
    lines_file    = [line.strip() for line in file_text.splitlines() if line.strip()]

    # Build list of sets for each source
    sets_current = [parse_line_to_set(l) for l in lines_current]
    sets_file    = [parse_line_to_set(l) for l in lines_file]

    # Compare line‑by‑line sets
    with open(logs_path, "w", encoding="utf-8") as f, redirect_stdout(f):
      for i, (a, b) in enumerate(zip(sets_current, sets_file), start=1):
        same = a == b 
        if not same:
            print("mismatching line ", i)
            print("  expects:", b)  
            print("  current:", a)

    # If file is empty, remove it
    if os.path.getsize(logs_path) == 0:
      os.remove(logs_path)

    return
#-----------------------------------------------------------------------------------
# cmd_comp

#-----------------------------------------------------------------------------------
# cmd_comp1
#-----------------------------------------------------------------------------------
def cmd_comp1(cmd_name: str, catapp: CatiaApp, app: Application) -> None: 

    file_path = os.path.join(out_path, f"{cmd_name}.txt") 
    logs_path = os.path.join(log_path, f"{cmd_name}.log") 
    dlg = get_cmd_dlg(cmd_name, catapp)     
    dlg_spec = app.window(handle=dlg.handle)


    buf = io.StringIO()
    with redirect_stdout(buf):
      dlg_spec.print_control_identifiers(depth=2)

    identifiers_text = buf.getvalue()

    
    # try to click close button
    close_dialog(dlg)

    # cancel dlg
    try:
        dlg.type_keys("{ESC}")  
    except Exception:
        dlg.type_keys("{ENTER}")   # fallback 

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            file_text = f.read()
    except FileNotFoundError:
        file_text = ""        # or None
        print(f"⚠ Skipping missing file: {file_path}")
        with open(logs_path, "w", encoding="utf-8") as f, redirect_stdout(f):
          print("missing reference file: ", file_path)
          return

    # Normalize and split
    lines_current = [line.strip() for line in identifiers_text.splitlines() if line.strip()]
    lines_file    = [line.strip() for line in file_text.splitlines() if line.strip()]

    # Build list of sets for each source
    sets_current = [parse_line_to_set(l) for l in lines_current]
    sets_file    = [parse_line_to_set(l) for l in lines_file]

    # Compare line‑by‑line sets
    with open(logs_path, "w", encoding="utf-8") as f, redirect_stdout(f):
      for i, (a, b) in enumerate(zip(sets_current, sets_file), start=1):
        same = a == b 
        if not same:
            print("mismatching line ", i+1)
            print("  expects:", b)  
            print("  current:", a)

    # If file is empty, remove it
    if os.path.getsize(logs_path) == 0:
      os.remove(logs_path)

    return
#-----------------------------------------------------------------------------------
# cmd_comp1

#-----------------------------------------------------------------------------------
# read_file_lines
#-----------------------------------------------------------------------------------
def read_file_lines(file_path: str, encoding: str = "utf-8") -> list[str]:
    """
    Read a text file and return its non‑empty lines as a list of strings.

    Args:
        file_path (str): Path to the file to read.
        encoding (str):  Text encoding (default 'utf-8').

    Returns:
        list[str]: All non‑empty lines with line breaks stripped.
    """
    try:
        with open(file_path, "r", encoding=encoding) as f:
            lines = [
                line.strip()           # remove leading/trailing spaces/newlines
                for line in f
                if line.strip() 
                and not line.lstrip().startswith("#")        # keep only lines that are not empty
            ]
        return lines
    except FileNotFoundError:
        print(f"⚠ File not found: {file_path}")
        return []
    except Exception as e:
        print(f"⚠ Error reading {file_path}: {e}")
        return []
#-----------------------------------------------------------------------------------
# read_file_lines

#-----------------------------------------------------------------------------------
# cmd_shot
#-----------------------------------------------------------------------------------
def cmd_shot(cmd_name: str, catapp: CatiaApp, app: Application) -> None:
    """Dump CATIA command dialog identifiers (each only once)."""
    dlg = get_cmd_dlg(cmd_name, catapp)
    
    img = dlg.capture_as_image()        # returns a Pillow Image
    img.save(r"c:\\tmp\\dialog.png")
    print("Saved dialog.png")

    prompt = (
        "Describe all visible GUI windows, buttons, and text "
        "that you can see in this screenshot. "
        "Summarize any alerts or abnormal states."
    )

    img_desc = analyze_screenshot(img, prompt)
    print(img_desc)

    prompt1 = "Is green lock button in 'Symmetry Info' unlocked?"
    # prompt1 = "Which entries are contained in the Group column?"
    img_desc1 = analyze_screenshot(img,prompt1)
    print(img_desc1)

    # prompt2 = "Is 'OK' button enabled?"
    # mg_desc2 = analyze_screenshot(img,prompt2)
    # print(img_desc2)

    # try to click close button
    if close_dialog(dlg):
      return;

    # Close the dialog
    try:
        dlg.type_keys("{ESC}")
    except Exception:
        dlg.type_keys("{ENTER}")

    return;
#-----------------------------------------------------------------------------------
# cmd_shot

#-----------------------------------------------------------------------------------
# analyze_screenshot
#-----------------------------------------------------------------------------------
def analyze_screenshot(image, prompt:str):
    # Convert image to base64
    buf = io.BytesIO()
    image.save(buf, format="PNG")
    img_b64 = base64.b64encode(buf.getvalue()).decode("utf-8")
        
    response = client.chat.completions.create(
        # model="gpt-4o",   
        model="gpt-5-chat-latest",   
        # model="gpt-image-1",
        messages=[
            {
                "role": "user",
                "content": [
                    {"type": "text", "text": prompt},
                    {                           # 👇 the important part
                        "type": "image_url",
                        "image_url": {
                            "url": f"data:image/png;base64,{img_b64}"
                        }
                    }
                ],
            }
        ],
    )  

    # Get the base64 image, decode, and save
    #  img_b64 = resp.data[0].b64_json
    # with open(r"c:\tmp\result.png", "wb") as f:
    #    f.write(base64.b64decode(img_b64))

    return response.choices[0].message.content
#-----------------------------------------------------------------------------------
# analyze_screenshot

#-----------------------------------------------------------------------------------
# test
#-----------------------------------------------------------------------------------
if __name__ == '__main__':

    dispatch = win32com.client.dynamic._GetGoodDispatch("CATIA.Application")
    typeinfo = dispatch.GetTypeInfo()
    attr = typeinfo.GetTypeAttr()
    olerepr = win32com.client.build.DispatchItem(typeinfo, attr, None, 0) 

    catapp = win32com.client.Dispatch("CATIA.Application")  

    # connect to running CATIA session
    app = Application().connect(title_re="CATIA V5*")

    catia = app.window(title_re="CATIA V5.*")
    img = catia.capture_as_image()

    img.save(r"c:\tmp\catia_main.png")

    prompt2 = (
        "Describe the 3d model "
        "that you can see in this screenshot. " 
        "Describe each colored object according to its function!"
    )

    prompt = ( 
        "Describe the red object of the 3d model in the given screenshot according to its function. " 
    )

    img_desc = analyze_screenshot(img, prompt)
    print(img_desc)
    

    prompt1 = (
        "Describe the both 3d Models "
        "that you can see in this screenshot. "
        "What are the differences between them? "
        "Assign each colored face from the left model to the according one in the right model and show the assignment in table form!"
    )
    

    # resp = client.images.generate(
    #    model="gpt-image-1",                     # image‑generation model        
    #    prompt="Draw a simple icon representing a gear."
    # )

    # resp = client.images.edit(
    #    model="gpt-image-1",
    #    image=open(r"c:\tmp\catia_main.png", "rb"),
    #    prompt="Highlight the OK button in red outline."
    # ) 
   
    # catapp.StartWorkbench("TCAMTAWorkbenchImpl") 
    # catapp.StartWorkbench("PrsConfiguration") 

    # print all comnads of CATIA applications
    # commands = dir(catapp)
    # print(commands)
    
    # print id of the current workbench
    print(catapp.GetWorkbenchId())
    

    # delte all logs
    delete_all_contents(log_path) 
    
    # cmd_name = "VTAXML"
    # cmd_name = "VTA-Table"
    cmd_name = "MTA-GapFlush"
    # cmd_name = "Variants Management"
    #cmd_shot( cmd_name, catapp, app) 

    # cmd_dump( cmd_name, catapp, app) 
    # cmd_comp( cmd_name, catapp, app) 

    # cmd_dump( "MTAInitPartCommand", catapp, app)  
    # cmd_dump( "MTAInitSPMCommand", catapp, app) 
    # cmd_comp( "MTAInitPartCommand", catapp, app) 
    # cmd_comp( "MTAInitSPMCommand", catapp, app)  
    
    # teast all MTA-Workbench commands
    cmd_names_mta_wkb = read_file_lines(cmd_mta_wkb_path)
    print(f"Read {len(cmd_names_mta_wkb)} lines:")
    for i, line in enumerate(cmd_names_mta_wkb, start=1):
        print(f"{i:03d}: {line}")
        # cmd_dump(line, catapp, app) 
        # cmd_comp(line, catapp, app)  
#-----------------------------------------------------------------------------------
# test