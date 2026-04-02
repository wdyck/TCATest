import cv2
import numpy as np  
import os 
 
from pathlib import Path
from PIL import Image
from itertools import product
import itertools
from scipy.optimize import linear_sum_assignment
import scipy
print(scipy.__version__)

# --------------------------------------------------------------------------------------------------
# compare_red_regions
# --------------------------------------------------------------------------------------------------
import cv2
import numpy as np

def compare_red_regions(img1_path, img2_path, size=(3000, 3000), similarity_threshold=0.7):
    """
    Compare red‑colored regions of two images.
    Returns IoU (intersection‑over‑union) similarity score between red areas.
    Automatically resizes both images to 'size' (default 3000×3000).

    Parameters
    ----------
    img1_path : str
        Path to first image.
    img2_path : str
        Path to second image.
    size : tuple(int, int)
        Target resize dimensions (width, height).
    similarity_threshold : float
        Optional threshold to decide if images are similar.

    Returns
    -------
    float
        IoU similarity (0.0 – 1.0).
    bool
        True if IoU >= similarity_threshold.
    """

    img1 = cv2.imread(img1_path)
    img2 = cv2.imread(img2_path)
    if img1 is None or img2 is None:
        raise ValueError(f"❌ Could not read one of the images: {img1_path}, {img2_path}")

    # 1️⃣ Resize both to the same 3000×3000 for consistent comparison
    img1 = cv2.resize(img1, size, interpolation=cv2.INTER_AREA)
    img2 = cv2.resize(img2, size, interpolation=cv2.INTER_AREA)

    # 2️⃣ Convert to HSV color space for reliable red detection
    hsv1 = cv2.cvtColor(img1, cv2.COLOR_BGR2HSV)
    hsv2 = cv2.cvtColor(img2, cv2.COLOR_BGR2HSV)

    # 3️⃣ Hue ranges for "red" in HSV
    lower_red1, upper_red1 = np.array([0, 70, 50]), np.array([10, 255, 255])
    lower_red2, upper_red2 = np.array([170, 70, 50]), np.array([180, 255, 255])

    mask1 = cv2.inRange(hsv1, lower_red1, upper_red1) | cv2.inRange(hsv1, lower_red2, upper_red2)
    mask2 = cv2.inRange(hsv2, lower_red1, upper_red1) | cv2.inRange(hsv2, lower_red2, upper_red2)

    # 4️⃣ Clean the masks (remove noise, close small gaps)
    kernel = np.ones((5, 5), np.uint8)
    mask1 = cv2.morphologyEx(mask1, cv2.MORPH_OPEN, kernel)
    mask1 = cv2.morphologyEx(mask1, cv2.MORPH_CLOSE, kernel)
    mask2 = cv2.morphologyEx(mask2, cv2.MORPH_OPEN, kernel)
    mask2 = cv2.morphologyEx(mask2, cv2.MORPH_CLOSE, kernel)

    # 5️⃣ Compute IoU (intersection over union)
    intersection = np.logical_and(mask1 > 0, mask2 > 0)
    union = np.logical_or(mask1 > 0, mask2 > 0)
    iou = intersection.sum() / union.sum() if union.sum() > 0 else 0.0

    return iou, iou >= similarity_threshold
# --------------------------------------------------------------------------------------------------
# compare_red_regions
# --------------------------------------------------------------------------------------------------

# Example usage:
# iou, is_similar = compare_red_regions("imgA.png", "imgB.png")
# print(f"IoU = {iou:.3f}, similar? {is_similar}")

import cv2
import numpy as np
import os

# --------------------------------------------------------------------------------------------------
# compare_red_contours
# --------------------------------------------------------------------------------------------------
import os
import cv2
import numpy as np
from typing import Tuple, Optional, List

# --------------------------------------------------------------------------------------------------
# compare_red_contours
# --------------------------------------------------------------------------------------------------
def compare_red_contours(
    img_path_1: str,
    img_path_2: str,
    visualize: bool = False
) -> Tuple[Tuple[Optional[np.ndarray], Optional[np.ndarray]], float, float]:
    """
    Detects and compares red-colored contours between two images.

    The function finds all red regions in both images, extracts their contours,
    and computes pairwise shape similarity using Hu moment matching.
    It returns the best-matching contour pair and the corresponding similarity score.

    Args:
        img_path_1: Path to the first image.
        img_path_2: Path to the second image.
        visualize:  If True, show both images with the best-matching
                    contours highlighted in green.

    Returns:
        best_pair:  (contour_from_img1, contour_from_img2) or (None, None)
        best_score: Raw cv2.matchShapes distance (0 = identical).
        similarity: Float in [0, 1], higher = more similar.
    """

    def extract_red_contours(path: str) -> Tuple[np.ndarray, List[np.ndarray]]:
        """Extract significant red contours from an image path."""
        img = cv2.imread(path)
        if img is None:
            raise FileNotFoundError(f"Cannot load image: {path}")

        hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

        # Red hue appears in two ranges (~0° and ~180°)
        lower1, upper1 = np.array([0, 70, 50]), np.array([10, 255, 255])
        lower2, upper2 = np.array([170, 70, 50]), np.array([180, 255, 255])

        mask = cv2.inRange(hsv, lower1, upper1) | cv2.inRange(hsv, lower2, upper2)

        # Morphological cleanup
        kernel = np.ones((3, 3), np.uint8)
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
        mask = cv2.morphologyEx(mask, cv2.MORPH_DILATE, kernel)

        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        # Filter out noise
        contours = [c for c in contours if cv2.contourArea(c) > 100]
        return img, contours

    # --- Extract contours from both images ---
    img1, cnts1 = extract_red_contours(img_path_1)
    img2, cnts2 = extract_red_contours(img_path_2)

    if not cnts1 or not cnts2:
        return (None, None), float("inf"), 0.0

    # --- Compare shapes using Hu moment distance ---
    best_pair, best_score = (None, None), float("inf")
    for c1 in cnts1:
        for c2 in cnts2:
            score = cv2.matchShapes(c1, c2, cv2.CONTOURS_MATCH_I1, 0.0)
            if score < best_score:
                best_score, best_pair = score, (c1, c2)

    # --- Visualization (optional) ---
    if visualize and all(best_pair):
        disp1, disp2 = img1.copy(), img2.copy()
        cv2.drawContours(disp1, [best_pair[0]], -1, (0, 255, 0), 2)
        cv2.drawContours(disp2, [best_pair[1]], -1, (0, 255, 0), 2)

        try:
            cv2.imshow("Best Match - Image 1", disp1)
            cv2.imshow("Best Match - Image 2", disp2)
            cv2.waitKey(0)
            cv2.destroyAllWindows()
        except cv2.error:
            print("⚠️ Visualization not supported in this environment.")

    # Convert distance to similarity [0, 1]
    similarity = float(np.clip(1.0 - best_score, 0.0, 1.0))
    return best_pair, best_score, similarity
# --------------------------------------------------------------------------------------------------

# --------------------------------------------------------------------------------------------------
# compare_against_directory
# --------------------------------------------------------------------------------------------------
def compare_against_directory(
    reference_img: str,
    folder: str,
    threshold: float = 0.7
) -> Optional[str]:
    """
    Compare one reference image to all images in a folder using red-contour similarity.

    Returns the file path of the best-matching image.

    Args:
        reference_img: Path to reference image.
        folder:        Folder containing candidate images.
        threshold:     Minimum similarity for a "good" match (0–1).

    Returns:
        Path to best-matching image, or None if no valid match.
    """
    if not os.path.isdir(folder):
        raise NotADirectoryError(f"Directory not found: {folder}")

    results = []
    for fname in os.listdir(folder):
        if fname.lower().endswith((".png", ".jpg", ".jpeg")):
            target_path = os.path.join(folder, fname)
            try:
                _, _, similarity = compare_red_contours(reference_img, target_path)
                results.append((target_path, similarity))
            except Exception as e:
                print(f"⚠️ Error comparing '{fname}': {e}")

    if not results:
        print("No valid images found.")
        return None

    # Sort by similarity, descending
    results.sort(key=lambda x: x[1], reverse=True)
    best_path, best_similarity = results[0]

    print("\n🏆 Best match (based on red contours):")
    print(f"{best_path}  (similarity = {best_similarity:.3f})")
    if best_similarity >= threshold:
        print("✅ Contours reasonably similar.")
    else:
        print("❌ No sufficiently similar contours found.")

    return best_path
# --------------------------------------------------------------------------------------------------
# -------------------------------------------------------------------------------------------------- 

#-----------------------------------------------------------------------------------
# combine_images_horizontally
#-----------------------------------------------------------------------------------
def combine_images_horizontally(path_to_image1: str, path_to_image2: str, path_result_image: str):
    """
    Combines two images side by side (horizontally) and saves the result.

    Parameters
    ----------
    path_to_image1 : str
        Path to the first image.
    path_to_image2 : str
        Path to the second image.
    path_result_image : str
        Path (including filename) where the combined image will be saved.

    Returns
    -------
    None (the combined image is saved to disk)
    """

    # Open both images
    img1 = Image.open(path_to_image1)
    img2 = Image.open(path_to_image2)

    # Ensure both images have the same height (optional)
    if img1.height != img2.height:
        # Resize second image to match the first one’s height (maintaining aspect ratio)
        new_width = int(img2.width * img1.height / img2.height)
        img2 = img2.resize((new_width, img1.height))

    # Calculate dimensions of the combined image
    total_width = img1.width + img2.width
    max_height = max(img1.height, img2.height)

    # Create a new blank canvas
    combined = Image.new("RGBA", (total_width, max_height), (255, 255, 255, 0))

    # Paste images one after another horizontally
    combined.paste(img1, (0, 0))
    combined.paste(img2, (img1.width, 0))

    # Save the result
    combined.save(path_result_image)
    print(f"✅ Combined image saved at: {path_result_image}")
#-----------------------------------------------------------------------------------
# combine_images_horizontally



# -------------------------------------------------------------------------
# detect_red_areas
# -------------------------------------------------------------------------
def detect_red_areas(image_path, save_debug=False):
    img = cv2.imread(str(image_path))
    if img is None:
        raise FileNotFoundError(image_path)

    # In HSV konvertieren (robuster gegen Helligkeitsänderungen)
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

    # Rot kann am Anfang und Ende des Hue‑Spektrums liegen:
    lower_red1 = np.array([0, 80, 80])
    upper_red1 = np.array([10, 255, 255])
    lower_red2 = np.array([160, 80, 80])
    upper_red2 = np.array([180, 255, 255])

    mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
    mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
    mask = cv2.bitwise_or(mask1, mask2)

    # Optional: kleine Pixelreste filtern
    kernel = np.ones((3,3), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=2)

    # Konturen finden
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    red_regions = []
    for c in contours:
        area = cv2.contourArea(c)
        if area > 100:  # zu kleine Bereiche ignorieren
            x,y,w,h = cv2.boundingRect(c)
            red_regions.append({"bbox": (x,y,w,h), "area": area})
            if save_debug:
                cv2.rectangle(img, (x,y), (x+w, y+h), (0,255,0), 2)

    if save_debug:
        debug_path = Path(image_path).with_name(f"{Path(image_path).stem}_detected.png")
        cv2.imwrite(str(debug_path), img)
        print(f"Debug image saved to {debug_path}")

    return red_regions
# -------------------------------------------------------------------------
# detect_red_areas

# -------------------------------------------------------------------------
# has_red_area
# -------------------------------------------------------------------------
def has_red_area(image_path, threshold_ratio=0.001):
    """Gibt True zurück, wenn mindestens ein roter Bereich im Bild vorkommt."""
    img = cv2.imread(image_path)
    if img is None:
        raise FileNotFoundError(image_path)

    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

    # Rot liegt an beiden Enden des Hue‑Spektrums
    lower1, upper1 = np.array([0,  80, 80]),  np.array([10, 255, 255])
    lower2, upper2 = np.array([160,80, 80]),  np.array([180,255,255])

    mask1 = cv2.inRange(hsv, lower1, upper1)
    mask2 = cv2.inRange(hsv, lower2, upper2)
    mask = cv2.bitwise_or(mask1, mask2)

    red_pixels = np.count_nonzero(mask)
    total_pixels = mask.size
    ratio = red_pixels / total_pixels

    return ratio > threshold_ratio   # True = mindestens etwas Rot vorhanden
# -------------------------------------------------------------------------
# has_red_area
 
# usage example
reference_dir = r"C:\tmp\doors\old" 
target_dir = r"C:\tmp\doors\new"  
output_dir = r"C:\tmp\doors\best_matches"  

# -------------------------------------------------------------------------
# combine_best_match_images
# -------------------------------------------------------------------------
def combine_best_match_images(reference_dir, target_dir, output_dir):
  for ref_fname in os.listdir(reference_dir):
      if not ref_fname.lower().endswith((".png", ".jpg", ".jpeg")):
          continue

      reference_path = os.path.join(reference_dir, ref_fname)
      print(f"\n🔍 Processing: {ref_fname}")

      # 1️⃣ Find best match in target_dir
      best_match_path = compare_against_directory(reference_path, target_dir, threshold=0.7)
      if best_match_path is None:
          print("⚠️ No valid match found, skipping.")
          continue

      # 2️⃣ Build output file name using both input filenames
      best_fname = os.path.basename(best_match_path)
      result_name = f"{os.path.splitext(ref_fname)[0]}__{os.path.splitext(best_fname)[0]}.png"
      result_path = os.path.join(output_dir, result_name)

      # 3️⃣ Combine reference and best match horizontally
      combine_images_horizontally(reference_path, best_match_path, result_path)

      print(f"✅ Combined image saved at: {result_path}")

  # same_region = compare_red_regions(path1, path2)
  # print("Same area highlighted:" if same_region else "Different areas highlighted!")
  print( "done")
# -------------------------------------------------------------------------
# combine_best_match_images

# -----------------------------------------------------------------------------
# Utility: extract contour features from a single image
# -----------------------------------------------------------------------------
def contour_features(path):
    img = cv2.imread(path)
    if img is None:
        raise IOError(f"Cannot read {path}")

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    # mask the red region (target contour might be red)
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    lower1 = np.array([0, 70, 50]); upper1 = np.array([10, 255, 255])
    lower2 = np.array([170, 70, 50]); upper2 = np.array([180, 255, 255])
    mask = cv2.inRange(hsv, lower1, upper1) | cv2.inRange(hsv, lower2, upper2)

    # fallback: use threshold if no red mask found
    if cv2.countNonZero(mask) < 10:
        _, mask = cv2.threshold(gray, 128, 255, cv2.THRESH_BINARY)

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:
        raise ValueError(f"No contour found in {path}")

    # Largest contour should correspond to the highlighted hole
    c = max(contours, key=cv2.contourArea)
    M = cv2.moments(c)
    if M["m00"] != 0:
        cx = M["m10"] / M["m00"]
        cy = M["m01"] / M["m00"]
    else:
        cx, cy = 0, 0
    area = cv2.contourArea(c)
    hu = cv2.HuMoments(M).flatten()
    hu = -np.sign(hu) * np.log10(np.abs(hu) + 1e-30)  # scale-friendly
    return {"centroid": (cx, cy), "area": area, "hu": hu, "contour": c}

# -----------------------------------------------------------------------------
# Compute similarity score between two contour feature dicts
# -----------------------------------------------------------------------------
def contour_distance(f1, f2, w_shape=1.0, w_area=0.3, w_pos=0.3):
    # smaller is better
    shape_score = cv2.matchShapes(f1["contour"], f2["contour"], cv2.CONTOURS_MATCH_I3, 0)
    area_score = abs(f1["area"] - f2["area"]) / (max(f1["area"], f2["area"]) + 1e-9)
    dx = f1["centroid"][0] - f2["centroid"][0]
    dy = f1["centroid"][1] - f2["centroid"][1]
    pos_score = np.sqrt(dx*dx + dy*dy)
    # combine (normalize position roughly)
    return w_shape*shape_score + w_area*area_score + w_pos*(pos_score/100.0)

# -----------------------------------------------------------------------------
# Main matching logic
# -----------------------------------------------------------------------------
def match_image_sets(dir_A, dir_B):
    imgs_A = sorted([os.path.join(dir_A, f) for f in os.listdir(dir_A)
                     if f.lower().endswith((".png", ".jpg", ".jpeg"))])
    imgs_B = sorted([os.path.join(dir_B, f) for f in os.listdir(dir_B)
                     if f.lower().endswith((".png", ".jpg", ".jpeg"))])
    features_A = [contour_features(p) for p in imgs_A]
    features_B = [contour_features(p) for p in imgs_B]

    cost_matrix = np.zeros((len(imgs_A), len(imgs_B)), dtype=float)
    for i, j in product(range(len(imgs_A)), range(len(imgs_B))):
        cost_matrix[i, j] = contour_distance(features_A[i], features_B[j])

    rows, cols = linear_sum_assignment(cost_matrix)  # optimal mapping
    matches = [(imgs_A[r], imgs_B[c], cost_matrix[r, c]) for r, c in zip(rows, cols)]
    return sorted(matches, key=lambda x: x[2])   # sort by cost ascending

# -----------------------------------------------------------------------------
# Example Usage
# -----------------------------------------------------------------------------
if __name__ == "__main__": 
    matches = match_image_sets(reference_dir, target_dir)
    print("Best matches (A -> B):\n")
    for a, b, score in matches:
        print(f"{os.path.basename(a):30s} ↔  {os.path.basename(b):30s}   score={score:.4f}")

        # 2️⃣ Build output file name using both input filenames 
        result_name = f"{os.path.splitext(os.path.basename(a))[0]}__{os.path.splitext(os.path.basename(b))[0]}.png"
        result_path = os.path.join(output_dir, result_name)

        # 3️⃣ Combine reference and best match horizontally
        combine_images_horizontally(a, b, result_path)

        print(f"✅ Combined image saved at: {result_path}")