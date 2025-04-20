import os
import zipfile
import subprocess
import time
import threading

MAIN_ZIP = "main.zip"  # Change if needed
EXTRACT_DIR = "extracted"
FINAL_OUTPUT_FILE = "final_output.txt"

# Flags to control skipping
skip_current_py = False
skip_current_zip = False

def user_input_listener():
    global skip_current_py, skip_current_zip
    while True:
        user_input = input().strip().lower()
        if user_input == "n":
            skip_current_py = True  # Skip only the current Python file
        elif user_input == "z":
            skip_current_zip = True  # Skip the entire zip folder

# Start listening for user input in a separate thread
threading.Thread(target=user_input_listener, daemon=True).start()

# Step 1: Extract the main zip
if not os.path.exists(EXTRACT_DIR):
    os.makedirs(EXTRACT_DIR)

with zipfile.ZipFile(MAIN_ZIP, 'r') as zip_ref:
    zip_ref.extractall(EXTRACT_DIR)

# Step 2: Process each subzip
final_output = []
for root, _, files in os.walk(EXTRACT_DIR):
    for file in files:
        if file.endswith(".zip"):
            subzip_name = file[:-4]  # Remove ".zip" extension
            print(f"Processing {subzip_name}.zip... (Press 'z' to skip this zip)")

            # Reset skip flag for the current zip
            skip_current_zip = False  
            subzip_path = os.path.join(root, file)
            subzip_extract_path = os.path.join(root, subzip_name)

            with zipfile.ZipFile(subzip_path, 'r') as subzip:
                subzip.extractall(subzip_extract_path)

            if skip_current_zip:  # Skip this zip folder if requested
                print(f"Skipping {subzip_name}.zip...\n")
                continue  

            # Step 3: Find and execute all Python files
            subzip_output = [subzip_name]  
            for subroot, _, subfiles in os.walk(subzip_extract_path):
                for subfile in subfiles:
                    if subfile.endswith(".py"):
                        py_path = os.path.join(subroot, subfile)
                        output_file_path = os.path.join(subroot, "output.txt")

                        print(f"Executing {subfile} inside {subzip_name}... (Press 'n' to skip this file)")

                        # Reset skip flag for current Python file
                        skip_current_py = False  

                        # Execute first command
                        if not skip_current_py:
                            command_1 = ["python", py_path, "iterative", "iitk.ac.in"]
                            try:
                                result_1 = subprocess.run(command_1, capture_output=True, text=True, timeout=30)
                                output_content = f"Command 1 Output:\n{result_1.stdout.strip()}\n"
                            except subprocess.TimeoutExpired:
                                output_content = "Command 1 Timeout.\n"
                            except Exception as e:
                                output_content = f"Command 1 Error:\n{str(e)}\n"
                        else:
                            print(f"Skipping {subfile}...\n")
                            continue  # Skip to the next Python file

                        # Wait 5 seconds before executing the second command
                        time.sleep(5)

                        # Execute second command
                        if not skip_current_py:
                            command_2 = ["python", py_path, "recursive", "iitk.ac.in"]
                            try:
                                result_2 = subprocess.run(command_2, capture_output=True, text=True, timeout=30)
                                output_content += f"\nCommand 2 Output:\n{result_2.stdout.strip()}\n"
                            except subprocess.TimeoutExpired:
                                output_content += "\nCommand 2 Timeout.\n"
                            except Exception as e:
                                output_content += f"\nCommand 2 Error:\n{str(e)}\n"
                        else:
                            print(f"Skipping {subfile}...\n")
                            continue  # Skip to the next Python file

                        # Save to output.txt
                        with open(output_file_path, "w") as out_f:
                            out_f.write(output_content)

                        subzip_output.append(output_content)

                        # Check if user pressed 'z' to skip the entire zip file
                        if skip_current_zip:
                            print(f"Skipping remaining files in {subzip_name}.zip...\n")
                            break

            # Step 4: Append to final output
            final_output.append("\n".join(subzip_output))

            print(f"Completed processing {subzip_name}.zip.\n")

# Step 5: Save the consolidated output
with open(FINAL_OUTPUT_FILE, "w") as final_out:
    final_out.write("\n\n".join(final_output))

print(f"All processing completed. Consolidated output saved in {FINAL_OUTPUT_FILE}")

