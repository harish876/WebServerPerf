use std::collections::HashMap;
use std::io::{ErrorKind, prelude::*};
use std::net::TcpStream;

use serde::{Deserialize, Serialize};
use serde_json::Result as JsonResult;

#[derive(Deserialize, Serialize, Debug)]
struct JsonBody {
    id: u32,
    value: String,
}

pub enum ConnectionStatus {
    Complete,
    WouldBlock,
    Error,
}

pub fn handle_connection(mut stream: &TcpStream) -> ConnectionStatus {
    let mut buff = [0u8; 4096];
    let bytes_read = match stream.read(&mut buff) {
        Ok(n) => n,
        Err(e) => {
            if e.kind() == ErrorKind::WouldBlock {
                return ConnectionStatus::WouldBlock;
            } else {
                eprintln!("Failed to read: {}", e);
                return ConnectionStatus::Error;
            }
        }
    };

    if bytes_read == 0 {
        return ConnectionStatus::Complete;
    }

    let request_str = match std::str::from_utf8(&buff[0..bytes_read]) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("Invalid UTF-8 sequence: {}", e);
            return ConnectionStatus::Error;
        }
    };

    // Parse headers and get content length
    let mut lines = request_str.lines();
    let request_line = match lines.next() {
        Some(line) => line,
        None => {
            eprintln!("Empty request");
            return ConnectionStatus::Complete;
        }
    };

    let parts: Vec<&str> = request_line.split_whitespace().collect();
    if parts.len() < 2 {
        eprintln!("Invalid request line: {}", request_line);
        stream.write_all(b"HTTP/1.1 400 Bad Request\r\n\r\n").ok();
        return ConnectionStatus::Complete;
    }

    let method = parts[0];
    let path = parts[1];

    let mut headers = HashMap::new();
    let mut content_length = 0;

    for line in lines {
        if line.is_empty() {
            break;
        }

        if let Some((key, value)) = line.split_once(':') {
            let key_lower = key.trim().to_lowercase();
            let value = value.trim().to_string();

            // Parse Content-Length specifically
            if key_lower == "content-length" {
                if let Ok(len) = value.parse::<usize>() {
                    content_length = len;
                }
            }

            headers.insert(key_lower, value);
        }
    }

    // Find body start
    let mut body = "";
    if let Some(body_index) = request_str.find("\r\n\r\n") {
        body = &request_str[body_index + 4..];

        // Validate body size against Content-Length
        if content_length > 0 && body.len() < content_length {
            // Body is incomplete - ideally we would read more data here
            eprintln!(
                "Incomplete body: received {} bytes but Content-Length is {}",
                body.len(),
                content_length
            );
            stream.write_all(b"HTTP/1.1 400 Bad Request\r\n\r\n").ok();
            return ConnectionStatus::Complete;
        }
    }

    // Process request
    let mut response = String::from("HTTP/1.1 404 Not Found\r\n\r\n");

    if method == "GET" && path.starts_with("/echo/") {
        if path.len() > 6 {
            let content = &path[6..];
            response = format!(
                "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}",
                content.len(),
                content
            );
        }
    } else if method == "POST" && path == "/json" {
        let content_type = headers
            .get("content-type") // Note: using lowercase key
            .cloned()
            .unwrap_or_else(|| "application/json".to_string());

        if content_type.contains("application/json") && !body.is_empty() {
            let json_result: JsonResult<JsonBody> = serde_json::from_str(body);

            match json_result {
                Ok(mut json_body) => {
                    json_body.id += 1;
                    if let Ok(modified_body) = serde_json::to_string(&json_body) {
                        response = format!(
                            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
                            modified_body.len(),
                            modified_body
                        );
                    } else {
                        response = "HTTP/1.1 500 Internal Server Error\r\n\r\n".to_string();
                    }
                }
                Err(e) => {
                    eprintln!("Failed to parse JSON body: {}", e);
                    response = "HTTP/1.1 400 Bad Request\r\n\r\n".to_string();
                }
            }
        } else {
            response = "HTTP/1.1 400 Bad Request\r\n\r\n".to_string();
        }
    } else if method == "GET" && path == "/" {
        response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello world!".to_string();
    }

    match stream.write_all(response.as_bytes()) {
        Ok(_) => {
            // Ensure data is sent
            if let Err(e) = stream.flush() {
                eprintln!("Failed to flush response: {}", e);
                return ConnectionStatus::Error;
            } else {
                return ConnectionStatus::Complete;
            }
        }
        Err(e) => {
            eprintln!("Failed to write response: {}", e);
            return ConnectionStatus::Error;
        }
    }
}
