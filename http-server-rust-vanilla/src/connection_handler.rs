use std::io::{BufReader, prelude::*};
use std::net::TcpStream;

pub trait HandleConnection {
    fn handle_connection(self);
}

impl HandleConnection for TcpStream {
    fn handle_connection(mut self) {
        let mut buf_reader = BufReader::new(&self);
        let mut buffer = String::new();

        // Read the request line
        if buf_reader.read_line(&mut buffer).is_err() {
            eprintln!("Failed to read line");
            return;
        }

        // Parse the request line
        let mut parts = buffer.split_whitespace();
        let method = parts.next().unwrap_or("");
        let path = parts.next().unwrap_or("");

        //println!("Method: {}, Path: {}", method, path);

        let mut response = String::from("HTTP/1.1 404 NotFound\r\n\r\n");
        if method == "GET" && path.contains("/echo/") {
            let content = &path[6..];
            let formatted_response = format!(
                "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}",
                content.len(),
                content
            );
            response = formatted_response;
        } else if method == "GET" && path == "/" {
            response = "HTTP/1.1 200 OK\r\n\r\n".to_string();
        }
        self.write_all(response.as_bytes()).unwrap();
    }
}
