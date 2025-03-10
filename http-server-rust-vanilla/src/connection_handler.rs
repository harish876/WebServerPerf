use std::collections::HashMap;
use std::io::{BufReader, prelude::*};
use std::net::TcpStream;

use serde::{Deserialize, Serialize};
use serde_json::Result as JsonResult;

pub trait HandleConnection {
    fn handle_connection(self);
}

#[derive(Deserialize, Serialize, Debug)]
struct JsonBody {
    id: u32,
    value: String,
}

pub fn handle_connection(mut stream: TcpStream) {
    let mut buf_reader = BufReader::new(&stream);
    let mut buffer = String::new();

    // Read the request line
    if buf_reader.read_line(&mut buffer).is_err() {
        eprintln!("Failed to read line");
        return;
    }

    // Parse the request line
    let request_line = buffer.clone();
    let mut parts = request_line.split_whitespace();
    let method = parts.next().unwrap_or("");
    let path = parts.next().unwrap_or("");

    let mut headers = HashMap::new();
    loop {
        buffer.clear();
        if buf_reader.read_line(&mut buffer).is_err() {
            println!("Failed to read header line");
            break;
        }
        if buffer == "\r\n" {
            break;
        }
        if let Some((key, value)) = buffer.split_once(':') {
            headers.insert(key.trim().to_string(), value.trim().to_string());
        }
    }

    let mut response = String::from("HTTP/1.1 404 NotFound\r\n\r\n");
    if method == "GET" && path.contains("/echo/") {
        let content = &path[6..];
        let formatted_response = format!(
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}",
            content.len(),
            content
        );
        response = formatted_response;
    } else if method == "POST" && path == "/json" {
        let content_length = headers
            .get("Content-Length")
            .and_then(|len| len.parse::<usize>().ok())
            .unwrap_or(0);

        let content_type = headers
            .get("Content-Type")
            .map(|s| s.to_string())
            .unwrap_or_else(|| "text/plain".to_string());

        let mut body = vec![0; content_length];
        if buf_reader.read_exact(&mut body).is_err() {
            eprintln!("Failed to read body");
        } else {
            let body_str = String::from_utf8_lossy(&body);
            if content_type == "application/json" {
                let json_result: JsonResult<JsonBody> = serde_json::from_str(&body_str);
                match json_result {
                    Ok(mut json_body) => {
                        json_body.id += 1;
                        let modified_body =
                            serde_json::to_string(&json_body).unwrap_or("".to_owned());
                        response = format!(
                            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
                            modified_body.len(),
                            modified_body
                        );
                        //println!("Response: {:?}", modified_body);
                    }
                    Err(e) => {
                        println!("Failed to parse JSON body: {}", e);
                        response = "HTTP/1.1 400 Bad Request\r\n\r\n".to_string();
                    }
                }
            }
        }
    } else if method == "GET" && path == "/" {
        response = "HTTP/1.1 200 OK\r\n\r\n".to_string();
    }
    stream.write_all(response.as_bytes()).unwrap();
}
