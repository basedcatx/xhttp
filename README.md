## Overview


XHTTP is a **tunneling proxy system** that establishes encrypted TCP connections between a local client and a remote server through a custom protocol. The system consists of two executables:


- **xhttp_c (Client)**: Runs on the local machine, listening on port 8090 for incoming connections from local applications (browsers, HTTP clients, etc.)
- **xhttp_s (Server)**: Runs on a remote machine, listening on port 8080 for connections from xhttp_c clients and forwarding traffic to an upstream proxy (default: localhost:3128)


**Purpose**: XHTTP enables secure, compressed communication through potentially restrictive networks by:
1. Encapsulating traffic in a custom packet protocol
2. Compressing data using zlib for bandwidth efficiency
3. Encrypting traffic with AES-256 GCM for confidentiality and integrity
4. Disguising encrypted packets with HTTP-like headers for potential firewall/proxy traversal


**Users**: Developers or network administrators who need to tunnel TCP traffic through monitored or restricted networks while maintaining security and optimizing bandwidth.


**Use Case Example**: A local application connects to `localhost:8090` → xhttp_c encrypts and forwards to remote server at `167.71.189.187:8080` → xhttp_s decrypts and forwards to squid proxy at `localhost:3128` → reaches final destination.


---


## Project Organization


### Build System Architecture


The project uses **CMake** as its build system, configured in `CMakeLists.txt`:


```
xhttp/
├── CMakeLists.txt          # Defines build targets and dependencies
├── CMakePresets.json       # CMake configuration presets
├── client/                 # Client executable source
│   └── Client.c            # Main client entry point and threading logic
├── server/                 # Server executable source
│   └── Server.c            # Main server entry point and threading logic
├── Encoder/                # Packet encoding/decoding
│   └── Encoder.c           # BufferEncode/Decode, compression, encryption
├── crypt/                  # Cryptography implementations
│   └── AES.c               # AES-256 GCM encryption/decryption
├── logger/                 # Logging subsystem
│   └── Logger.c            # Error logging functions
├── utils/                  # Networking utilities
│   ├── TcpClientUtility.c  # Client socket creation and connection
│   ├── TcpServerUtility.c  # Server socket creation and listening
│   ├── SocketUtility.c     # Non-blocking socket configuration
│   ├── AddressUtility.c    # Socket address printing
│   ├── Compressor.c        # zlib compression/decompression
│   └── Utils.c             # HTTP header generation
├── includes/               # Header files (public APIs)
│   ├── packet.h            # Packet structure and flags
│   ├── utils.h             # Utility function declarations
│   ├── crypt.h             # Cryptography API and key definition
│   └── logger.h            # Logging function declarations
└── tests/                  # Test executables
    └── test.c              # Integration test for encoding/decoding
```


### Core Systems


#### 1. **Client System** (`client/Client.c`)
- **Entry Point**: `main()` function
- **Core Function**: `handle_client_thread(void *args)`
- **Responsibilities**:
  - Listens on `DEF_LOCAL_PORT` (8090) for local connections
  - Spawns a new pthread for each accepted connection
  - Each thread establishes connection to `PROXY_HOST:PROXY_PORT` (167.71.189.187:8080)
  - Bidirectionally forwards data with custom protocol encoding
  - Uses `select()` for non-blocking I/O multiplexing


#### 2. **Server System** (`server/Server.c`)
- **Entry Point**: `main()` function
- **Core Function**: `handle_client_thread(void *args)`
- **Responsibilities**:
  - Listens on `DEF_LOCAL_PORT` (8080) for incoming xhttp_c connections
  - Spawns pthread per client connection
  - Establishes outbound connection to `PROXY_HOST:PROXY_PORT` (localhost:3128)
  - Decodes incoming packets and forwards raw data to upstream proxy
  - Encodes upstream responses back into packet format


#### 3. **Packet Protocol Layer** (`includes/packet.h`, `Encoder/Encoder.c`)
- **Key Structure**: `struct Packet`
  ```c
  struct Packet {
      uint32_t msgLength;    // Length of message payload
      uint32_t structSize;   // Size of Packet structure
      uint8_t flag;          // Protocol flags (compression, request/response, etc.)
      uint8_t message[BUFSIZ * 3];  // Actual payload (max ~24KB)
  };
  ```
- **Encoding Pipeline**: Packet → Serialize → Compress (zlib) → Encrypt (AES-GCM) → Frame (40-byte header)
- **Key Functions**:
  - `BufferEncode()`: Serializes, compresses, encrypts packet
  - `BufferDecode()`: Decrypts, decompresses, deserializes packet
  - `FrameToSocket()`: Adds 40-byte header and sends to socket
  - `FrameFromSocket()`: Receives framed data and extracts payload


#### 4. **Cryptography System** (`crypt/AES.c`, `includes/crypt.h`)
- **Algorithm**: AES-256 GCM (authenticated encryption)
- **Key Management**: Static key `AES_CRYPT_KEY` hardcoded in `includes/crypt.h`
- **Key Parameters**:
  - Key size: 32 bytes (256 bits)
  - IV size: 12 bytes (96 bits, randomly generated per operation)
  - Auth tag size: 16 bytes
- **Functions**:
  - `aes_gcm_encrypt()`: Encrypts plaintext, returns ciphertext with IV prepended
  - `aes_gcm_decrypt()`: Decrypts ciphertext, verifies authentication tag


#### 5. **Compression System** (`utils/Compressor.c`)
- **Library**: zlib
- **Configuration**: `Z_BEST_COMPRESSION` level
- **Functions**:
  - `zlib_compress_dynamic()`: Dynamically allocates compressed buffer
  - `zlib_decompress_dynamic()`: Dynamically allocates decompressed buffer


#### 6. **Network Utilities** (`utils/`)
- **TCP Client**: `CreateClientSocket()` - Establishes outbound connections
- **TCP Server**: `CreateServerSocket()` - Creates listening sockets, `AcceptTCPConnection()` - Accepts clients
- **Socket Configuration**: `set_nonblocking_socket()` - Enables non-blocking I/O
- **Options Set**: `SO_REUSEPORT`, `TCP_NODELAY`


#### 7. **Logging System** (`logger/Logger.c`, `includes/logger.h`)
- **Non-fatal**: `LogErrorWithReason()` - Logs error, continues execution
- **Fatal (custom)**: `LogErrorWithReasonX()` - Logs error, calls `exit(EXIT_FAILURE)`
- **Fatal (system)**: `LogSystemError()` - Uses `perror()`, calls `exit(EXIT_FAILURE)`
- All output goes to `stdout`


### Threading Model


Both client and server use identical concurrency patterns:
1. **Main Thread**: Runs `accept()` loop in `select()` for new connections
2. **Worker Threads**: Spawned via `pthread_create()` and immediately detached with `pthread_detach()`
3. **Per-Thread Resources**: Each thread manages two sockets (local + remote) with `select()` multiplexing
4. **Lifecycle**: Threads self-terminate when either socket closes; no coordination with main thread


### Data Flow


**Client → Server Direction**:
```
Local App → [Raw TCP Data] → Client.c → Create Packet → BufferEncode → 
Compress → Encrypt → FrameToSocket (40-byte header) → Network → 
Server.c → FrameFromSocket → Decrypt → Decompress → BufferDecode → 
Extract message → Forward to Proxy
```


**Server → Client Direction**:
```
Proxy Response → Server.c → Create Packet → BufferEncode → Compress → 
Encrypt → FrameToSocket → Network → Client.c → FrameFromSocket → 
Decrypt → Decompress → BufferDecode → Extract message → Forward to Local App
```


### Build Targets


- **xhttp_c**: Client executable (links all utilities + `client/Client.c`)
- **xhttp_s**: Server executable (links all utilities + `server/Server.c`)
- **test_c**: Test executable for encoding/decoding verification


### External Dependencies


- **ZLIB**: Data compression (`zlib_compress_dynamic`, `zlib_decompress_dynamic`)
- **OpenSSL::Crypto**: AES-GCM cryptographic operations
- **OpenSSL::SSL**: TLS/SSL support (linked but usage not visible in main code)
- **POSIX Threads**: Multi-threading (`pthread_create`, `pthread_detach`)
- **POSIX Sockets**: Network I/O (socket, bind, listen, accept, connect, select)


### Configuration


**Client Configuration** (`client/Client.c`):
```c
#define DEF_LOCAL_PORT "8090"              // Local listening port
#define PROXY_HOST "167.71.189.187"        // Remote server IP
#define PROXY_PORT "8080"                  // Remote server port
```


**Server Configuration** (`server/Server.c`):
```c
#define DEF_LOCAL_PORT "8080"              // Server listening port
#define PROXY_HOST "localhost"             // Upstream proxy host
#define PROXY_PORT "3128"                  // Upstream proxy port (Squid default)

---


## Glossary of Codebase-Specific Terms


### Core Architecture Terms


1. **xhttp_c**: Client executable that accepts local connections and tunnels them through encrypted protocol to xhttp_s. Built from `client/Client.c`. Listens on port 8090.


2. **xhttp_s**: Server executable that receives encrypted connections from xhttp_c and forwards to upstream proxy. Built from `server/Server.c`. Listens on port 8080.


3. **Packet**: Core data structure (`struct Packet` in `includes/packet.h`) containing `msgLength`, `structSize`, `flag`, and `message[BUFSIZ*3]`. Represents encapsulated application data.


4. **handle_client_thread**: Function in both `Client.c` and `Server.c` that manages bidirectional data forwarding for a single connection. Each runs in a detached pthread.


5. **DEF_LOCAL_PORT**: Port configuration macro. "8090" for client, "8080" for server. Defined in respective main files.


6. **PROXY_HOST/PROXY_PORT**: Destination configuration. Client: 167.71.189.187:8080 (xhttp_s). Server: localhost:3128 (Squid proxy).


### Protocol and Encoding


7. **BufferEncode**: Function in `Encoder/Encoder.c` that serializes Packet, compresses with zlib, encrypts with AES-GCM. Returns `uint8_t*` buffer.


8. **BufferDecode**: Function in `Encoder/Encoder.c` that reverses BufferEncode: decrypts, decompresses, deserializes into Packet structure.


9. **FrameToSocket**: Function in `Encoder/Encoder.c` that prepends 40-byte header to data and writes to socket. Used for protocol framing.


10. **FrameFromSocket**: Function in `Encoder/Encoder.c` that reads framed data from socket, skipping 40-byte header. Returns payload bytes.


11. **HEADER_SIZE**: Constant defined as 40 in `Encoder/Encoder.c`. Size of HTTP-like header prepended to all transmitted packets.


12. **msgLength**: Field in `struct Packet` storing length of actual message payload. Used for variable-length message handling.


13. **structSize**: Field in `struct Packet` storing `sizeof(struct Packet)`. Enables version compatibility checks or dynamic structure handling.


14. **flag**: Single-byte field in `struct Packet` for protocol flags (COMPRESSION_FLAG, IS_REQUEST_FLAG, etc.). Defined in `includes/packet.h`.


### Packet Flags


15. **COMPRESSION_FLAG**: Value 0x0100 in `includes/packet.h`. Indicates packet payload is compressed. Set during encoding pipeline.


16. **IS_REQUEST_FLAG**: Value 0x0500. Marks packet as client request. Used for protocol-level distinction between request/response.


17. **IS_RESPONSE_FLAG**: Value 0x0300. Marks packet as server response. Complementary to IS_REQUEST_FLAG.


18. **CONTINUATION_FLAG**: Value 0x0200. Indicates packet is part of multi-packet message sequence. For handling large payloads.


19. **IS_CHUNK_FLAG**: Value 0x0400. Denotes packet contains data chunk, possibly for streaming or progressive transfer.


### Cryptography


20. **AES_CRYPT_KEY**: Static string constant in `includes/crypt.h` containing hardcoded 256-bit encryption key. Used by all AES operations.


21. **aes_gcm_encrypt**: Function in `crypt/AES.c` that encrypts plaintext using AES-256 GCM. Generates random IV, prepends to ciphertext.


22. **aes_gcm_decrypt**: Function in `crypt/AES.c` that decrypts ciphertext, extracts IV, verifies authentication tag. Returns -1 on tag mismatch.


23. **AES_KEY_SIZE**: Constant 32 bytes (256 bits) in `crypt/AES.c`. Defines AES key length.


24. **AES_IV_SIZE**: Constant 12 bytes (96 bits) in `crypt/AES.c`. Initialization vector size for GCM mode.


25. **TAG_SIZE**: Constant 16 bytes in `crypt/AES.c`. Authentication tag size for GCM authenticated encryption.


### Compression


26. **zlib_compress_dynamic**: Function in `utils/Compressor.c` that compresses buffer using zlib Z_BEST_COMPRESSION. Dynamically allocates output.


27. **zlib_decompress_dynamic**: Function in `utils/Compressor.c` that decompresses zlib buffer. Dynamically resizes output as needed.


28. **Z_BEST_COMPRESSION**: zlib constant used in compression initialization. Maximizes compression ratio at cost of CPU.


### Networking Utilities


29. **CreateClientSocket**: Function in `utils/TcpClientUtility.c` that resolves hostname, creates socket, sets SO_REUSEPORT/TCP_NODELAY, connects.


30. **CreateServerSocket**: Function in `utils/TcpServerUtility.c` that creates, binds, and sets socket to listen with MAX_CONNECTED_SOCKS backlog.


31. **AcceptTCPConnection**: Function in `utils/TcpServerUtility.c` that monitors server socket with select() and accepts new client connections.


32. **set_nonblocking_socket**: Function in `utils/SocketUtility.c` that uses fcntl() to set O_NONBLOCK flag on socket descriptor.


33. **MAX_CONNECTED_SOCKS**: Constant 10 in `utils/TcpServerUtility.c`. Backlog parameter for listen() call, limits pending connection queue.


34. **STREAM_BUF_SIZE**: Macro `BUFSIZ * 3` in `includes/utils.h`. Size of I/O buffers (~24KB on most systems). Used for socket read/write.


35. **printSocketAddress**: Function in `utils/AddressUtility.c` that formats sockaddr to human-readable IP:port string. Used for logging.


36. **generate_http_header**: Function in `utils/Utils.c` that creates 40-byte HTTP-like header string. Used to disguise packets.


### Logging


37. **LogErrorWithReason**: Function in `logger/Logger.c` for non-fatal error logging. Outputs to stdout, continues execution.


38. **LogErrorWithReasonX**: Function in `logger/Logger.c` for fatal errors. Logs to stdout, calls exit(EXIT_FAILURE). 'X' suffix means "exit".


39. **LogSystemError**: Function in `logger/Logger.c` for system call failures. Uses perror(), then exits. For errno-based errors.


### Threading and I/O


40. **serverSock**: Global int in Client.c/Server.c. File descriptor for main listening socket created by CreateServerSocket().


41. **clntSock**: Local variable in thread functions. File descriptor for accepted client connection from AcceptTCPConnection().


42. **proxySocket**: Local variable in handle_client_thread. File descriptor for outbound connection to next hop (xhttp_s or Squid).


43. **client_buf/proxy_buf**: Local buffers of STREAM_BUF_SIZE in thread functions. Used for reading data from respective sockets.


44. **fd_set**: Standard type used with select() to monitor multiple socket descriptors. Declared as read_fd_set in thread loops.


45. **cleanup_handler**: Signal handler function in Client.c/Server.c. Registered for SIGINT/SIGTERM to call cleanup() on shutdown.


### Build System


46. **CMakePresets.json**: File defining CMake configuration presets for build, test, configure stages. Standardizes development environments.


47. **XHTTP**: Project namespace/prefix. Appears in header guards (XHTTP_PACKET_H, XHTTP_UTILS_H) and system identifier.


48. **temporal_buffer**: Local variable in Encoder.c encoding/decoding functions. Intermediate buffer between compression and encryption stages.


49. **HTTP_HEADER_TEMPLATE**: String constant in `includes/packet.h`: "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n". Template for generate_http_header().


50. **MIN_CONTENT_LENGTH/MAX_CONTENT_LENGTH**: Constants 100/999 in `includes/packet.h`. Constraints for HTTP header content-length field.
