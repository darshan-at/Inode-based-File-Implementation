## Inode Based File Implmentation

## Steps to Run the program
- Open the terminal from the folder where ```main.cpp``` exists
- Run the below command to compile the program
- ``` g++ main.cpp ```
- Run the following command to execute the executable file
- ``` ./a.out ```

## Features:
## Disk Creation and mounting
### 1: Create disk
- Enter the name of the disk
- Creates a disk of 500 MB

### 2: Mount disk
- Enter the name of the disk
- Reads the file's and disk's metadata to memory from disk

### 3: Exit
- Exits the program

## File Operations
### 1: Create file
- Enter the name of the file
### 2: Open file
- Enter the name of the file and mode in which to open
    0 - Read mode
    1 - Write mode
    2 -  Append mode
- Returns a file descriptor
### 3: Read file
- Enter the file descriptor returned by __Open file__ command
- Make sure you opened the file in Read mode
### 4: Write file
- Enter the file descriptor returned by __Open file__ command
- Make sure you opened the file in Write mode
- Enter the contents of the file
- Press ```CTRL + D``` to stop writing and save file.
### 5: Append file
- Enter the file descriptor returned by __Open file__ command
- Make sure you opened the file in Append mode
- Enter the contents of the file
- Press ```CTRL + D``` to stop writing and save file.
### 6: Close file
- Enter the file descriptor returned by __Open file__ command
### 7: Delete file
- Enter the file name
- Make sure you have closed the file
### 8: List of files
- Return the list of files
### 9: List of opened files
- Return the list of opened files
### 10: Unmount
- Writes the file's and disk's metadata back to disk
