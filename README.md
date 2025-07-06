# Network File System (NFS) Implementation

A distributed file system implementation with multiple storage servers, a central naming server, and client applications supporting various file operations including reading, writing, streaming, and backup functionality.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Components](#components)
- [Features](#features)
- [Installation and Setup](#installation-and-setup)
- [Usage](#usage)
- [File Operations](#file-operations)
- [Error Handling](#error-handling)
- [Advanced Features](#advanced-features)
- [Technical Details](#technical-details)
- [Troubleshooting](#troubleshooting)

## Overview

This Network File System (NFS) implementation provides a distributed file storage solution with the following key characteristics:

- **Distributed Architecture**: Multiple storage servers with a central naming server
- **High Availability**: Data replication and backup mechanisms
- **Concurrent Access**: Support for multiple clients with proper synchronization
- **Efficient Search**: Trie-based path resolution with LRU caching
- **Streaming Support**: Audio file streaming capabilities
- **Asynchronous Operations**: Non-blocking write operations for better performance

## Architecture

The NFS consists of three main components:

1. **Naming Server (NM)**: Central coordinator managing file location metadata
2. **Storage Servers (SS)**: Distributed storage nodes handling actual file operations
3. **Clients**: User applications requesting file operations

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Client 1  │    │   Client 2  │    │   Client N  │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       └──────────────────┼──────────────────┘
                          │
              ┌───────────▼───────────┐
              │   Naming Server (NM)  │
              │     Port: 8082        │
              └───────────┬───────────┘
                          │
       ┌──────────────────┼──────────────────┐
       │                  │                  │
┌──────▼──────┐    ┌──────▼──────┐    ┌──────▼──────┐
│ Storage SS1 │    │ Storage SS2 │    │ Storage SS3 │
│ Port: 8100  │    │ Port: 8102  │    │ Port: 8104  │
└─────────────┘    └─────────────┘    └─────────────┘
```

## Components

### 1. Naming Server (NM)

- **Port**: 8082 (configurable in `commonheaders.h`)
- **Responsibilities**:
  - Manages storage server registrations
  - Resolves file paths to storage servers
  - Handles client requests and routing
  - Maintains LRU cache for efficient lookups
  - Coordinates backup and replication

### 2. Storage Servers (SS)

- **Ports**: 8100, 8102, 8104, etc. (configurable)
- **Responsibilities**:
  - Store and manage actual files
  - Handle file operations (read, write, delete, etc.)
  - Maintain backup copies
  - Provide streaming capabilities
  - Register accessible paths with naming server

### 3. Client

- **Responsibilities**:
  - Send file operation requests
  - Handle responses from naming server
  - Direct communication with storage servers
  - Support for streaming audio files

## Features

### Core File Operations

- **READ**: Read file contents
- **WRITESYNC**: Synchronous file writing
- **WRITEASYNC**: Asynchronous file writing
- **CREATEFILE**: Create new files
- **CREATEFOLDER**: Create new directories
- **DELETEFILE**: Delete files
- **DELETEFOLDER**: Delete directories
- **COPYFILE**: Copy files between locations
- **COPYFOLDER**: Copy directories between locations
- **LIST**: List all accessible paths
- **INFO**: Get file information (size, permissions)
- **STREAM**: Stream audio files

### Advanced Features

- **Multiple Client Support**: Concurrent client access with proper synchronization
- **LRU Caching**: Efficient path resolution with caching
- **Data Replication**: Automatic backup across multiple storage servers
- **Failure Detection**: Heartbeat mechanism for storage server monitoring
- **Asynchronous Operations**: Non-blocking write operations
- **Error Handling**: Comprehensive error codes and messages
- **Logging**: Detailed operation logging for debugging

## Installation and Setup

### Prerequisites

- GCC or Clang compiler
- Python 3.x (for setup script)
- POSIX-compliant system (Linux/macOS)
- Make utility

### Quick Setup

1. **Clone the repository**:

   ```bash
   git clone <repository-url>
   cd NFS
   ```

2. **Run the setup script** (creates makefile and directories):

   ```bash
   python3 script.py <number_of_storage_servers>
   ```

   Example: `python3 script.py 3` creates 3 storage servers

3. **Build the project**:

   ```bash
   make
   ```

### Manual Setup

1. **Create storage server directories**:

   ```bash
   mkdir -p dir1/dir1 dir1/backup1
   mkdir -p dir2/dir2 dir2/backup2
   mkdir -p dir3/dir3 dir3/backup3
   ```

2. **Create accessible paths files**:

   ```bash
   echo "backup1" > dir1/accessible_paths.txt
   echo "dir1" >> dir1/accessible_paths.txt
   # Repeat for other directories
   ```

3. **Build components**:

   ```bash
   make client
   make nm
   make ss1 ss2 ss3
   ```

## Usage

### Starting the System

1. **Start the Naming Server**:

   ```bash
   ./nm
   ```

2. **Start Storage Servers** (in separate terminals):

   ```bash
   ./ss1 <naming_server_ip> dir1
   ./ss2 <naming_server_ip> dir2
   ./ss3 <naming_server_ip> dir3
   ```

3. **Start Client**:

   ```bash
   ./client <naming_server_ip>
   ```

### Example Session

```bash
# Start naming server
./nm

# Start storage servers (in separate terminals)
./ss1 127.0.0.1 dir1
./ss2 127.0.0.1 dir2
./ss3 127.0.0.1 dir3

# Start client
./client 127.0.0.1

# Client commands
Enter Command: CREATEFILE dir1 test.txt
Enter Command: WRITESYNC dir1/test.txt
Enter Command: READ dir1/test.txt
Enter Command: LIST
Enter Command: EXIT
```

## File Operations

### Reading Files

```bash
READ <path>
```

- Retrieves file contents from the appropriate storage server
- Supports text and binary files
- Returns file data directly to client

### Writing Files

```bash
WRITESYNC <path> <data>
WRITEASYNC <path> <data>
```

- **WRITESYNC**: Synchronous write, waits for completion
- **WRITEASYNC**: Asynchronous write, returns immediately
- Supports large files with chunked writing
- Automatic backup to multiple storage servers

### Creating Files and Folders

```bash
CREATEFILE <path> <filename>
CREATEFOLDER <path> <foldername>
```

- Creates empty files or directories
- Updates naming server's accessible paths
- Handles backup replication

### Copying Files and Folders

```bash
COPYFILE <source_path> <destination_path>
COPYFOLDER <source_path> <destination_path>
```

- Copies between different storage servers
- Maintains data consistency
- Updates path mappings

### File Information

```bash
INFO <path>
```

- Returns file size, permissions, timestamps
- Provides metadata about files and folders

### Listing Paths

```bash
LIST
```

- Shows all accessible paths across all storage servers
- Useful for discovery and navigation

### Streaming Audio

```bash
STREAM <audio_file_path>
```

- Streams audio files in binary format
- Compatible with command-line players (mpv, ffplay)
- Supports various audio formats

## Error Handling

The system implements comprehensive error handling with specific error codes:

- **File Not Found**: When requested file doesn't exist
- **Permission Denied**: When access is restricted
- **File in Use**: When file is being written by another client
- **Storage Server Down**: When storage server is unavailable
- **Invalid Path**: When path format is incorrect
- **Network Errors**: Connection and communication issues

## Advanced Features

### LRU Caching

- Caches recently accessed paths for faster resolution
- Configurable cache size (default: 10 entries)
- Thread-safe implementation with mutex protection

### Data Replication

- Automatic backup to multiple storage servers
- Asynchronous replication for write operations
- Failure recovery from backup copies

### Concurrent Access

- Multiple clients can read the same file simultaneously
- Write operations are exclusive (one client at a time)
- Proper synchronization using mutexes

### Heartbeat Mechanism

- Storage servers send periodic heartbeat signals
- Naming server detects failures automatically
- Enables failover to backup servers

### Asynchronous Operations

- Large file writes are handled asynchronously
- Immediate acknowledgment to clients
- Background processing with status updates

## Technical Details

### Data Structures

- **Trie**: Efficient path resolution and storage
- **LRU Cache**: Fast lookup for recent paths
- **Linked Lists**: Managing write operations and client connections
- **Queues**: Storage server communication

### Communication Protocol

- TCP sockets for reliable communication
- Structured message format with request types
- Binary data transfer for file operations
- JSON-like data structures for metadata

### Threading Model

- Multi-threaded naming server for concurrent client handling
- Separate threads for storage server communication
- Background threads for replication and cleanup

### File System Integration

- POSIX file operations
- Directory traversal and management
- File locking for concurrent access
- Metadata management

## Troubleshooting

### Common Issues

1. **Port Already in Use**:

   ```bash
   # Check for processes using the port
   netstat -tulpn | grep 8082
   # Kill the process
   kill -9 <process_id>
   ```

2. **Permission Denied**:

   ```bash
   # Ensure proper permissions
   chmod +x client nm ss1 ss2 ss3
   ```

3. **Connection Refused**:
   - Verify naming server is running
   - Check IP address and port configuration
   - Ensure firewall allows connections

4. **Build Errors**:

   ```bash
   # Clean and rebuild
   make clean
   make
   ```

### Debugging

1. **Enable Logging**: Check `logs.txt` for detailed operation logs
2. **Network Debugging**: Use `netstat` or `ss` to monitor connections
3. **Process Monitoring**: Use `ps` to check running processes
4. **File System**: Verify directory structure and permissions

### Performance Optimization

1. **Cache Size**: Adjust `MAX_LRU_SIZE` in `lru.h`
2. **Buffer Sizes**: Modify buffer sizes in `commonheaders.h`
3. **Thread Pool**: Adjust maximum clients in `namingserver.h`
4. **Network**: Optimize socket buffer sizes

## Assumptions

- File names contain "." (dot), folder names do not
- All file and folder names are unique
- No paths contain "backup" in their names
- Storage servers are on the same network as naming server
- TCP/IP networking is available
- POSIX-compliant file system operations

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

