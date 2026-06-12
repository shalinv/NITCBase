# NITCBase

This repository contains the work for the DBMS Lab, focusing on the development of **NITCBase**, a Relational Database Management System (RDBMS) implemented in C++ from scratch. The project is designed for educational purposes to demonstrate the core architectural components of a database system.

## Repository Structure

```text
NITCbase
├── Disk/                       
│   ├── disk                    
│   └── disk_run_copy           
├── Files/                      
│   ├── Batch_Execution_Files/  
│   ├── Input_Files/            
│   └── Output_Files/           
├── mynitcbase/                 
│   ├── Algebra/                
│   ├── BPlusTree/              
│   ├── BlockAccess/            
│   ├── Buffer/                  
│   ├── Cache/                  
│   ├── Disk_Class/             
│   ├── Frontend/               
│   ├── FrontendInterface/      
│   ├── Schema/                 
│   └── main.cpp                
├── XFS_Interface/                               
└── Dockerfile                  
```
## NITCBase Architecture

The codebase inside `NITCbase/mynitcbase/` is organized into several modules representing the typical layers of an RDBMS architecture:

- **Frontend / FrontendInterface**: Handles user interaction and parses database commands.
- **Algebra**: Implements relational algebra operations (e.g., Select, Project, Join).
- **Schema**: Manages the database schema (tables, attributes, metadata).
- **BlockAccess**: Handles read/write operations at the block level for relations and indices.
- **BPlusTree**: Implements the B+ Tree data structure for database indexing.
- **Cache**: Manages cached data and Open Relation Table structures in memory for faster access.
- **Buffer**: Implements the buffer manager to handle pages swapped between disk and memory.
- **Disk_Class**: Simulates or manages actual disk interactions and storage.

## Prerequisites

To build and run NITCBase natively, you will need:
- **C++ Compiler**: GCC/G++ supporting C++11 or later.
- **Make**: For building the project.
- **Linux-Based Environment**


Alternatively, you can use the provided `Dockerfile` to build and run the project in an isolated container.

## Build Instructions

Navigate to the NITCBase directory and use the `Makefile` for compilation.

```bash
cd NITCbase/mynitcbase
```

### Standard Build
To build the standard release version:
```bash
make
```
This will compile the source files and generate the `nitcbase` executable.

## Running NITCBase

After successfully building the project, you can run the interactive database shell:
```bash
./nitcbase
```

## Clean Up

To remove the compiled objects and executable files:
```bash
make clean
```
