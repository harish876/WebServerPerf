use epoll::{ControlOptions::*, Event, Events};
use serde::{Deserialize, Serialize};
use serde_json::Result as JsonResult;
use std::{
    collections::HashMap,
    io::{self, Read, Write},
    net::TcpListener,
    os::unix::io::AsRawFd,
};

#[derive(Debug, Clone)]
struct RequestContext {
    method: String,
    endpoint: String,
    content_length: usize,
    accept_encoding: Option<String>,
    content_type: Option<String>,
}

#[derive(Debug)]
enum ConnectionState {
    Read {
        request: Vec<u8>,
        read: usize,
    },
    Process {
        context: RequestContext,
        body: Vec<u8>,
    },
    Write {
        response: Vec<u8>,
        written: usize,
    },
    Flush,
}

#[derive(Deserialize, Serialize, Debug)]
struct JsonBody {
    id: u32,
    value: String,
}

pub fn use_threads(listener: TcpListener) {
    todo!("unimplemented");
}

pub fn use_thread_pool(listener: TcpListener) {
    todo!("unimplemented");
}

pub fn use_epoll(listener: TcpListener) -> io::Result<()> {
    let epoll = epoll::create(false).unwrap();

    let event = Event::new(Events::EPOLLIN, listener.as_raw_fd() as _);
    epoll::ctl(epoll, EPOLL_CTL_ADD, listener.as_raw_fd(), event).unwrap();

    let mut connections = HashMap::new();

    loop {
        let mut events = [Event::new(Events::empty(), 0); 1024];
        let timeout = -1; // block forever, until something happens
        let num_events = epoll::wait(epoll, timeout, &mut events).unwrap();

        let mut completed = Vec::new();
        'next: for event in &events[..num_events] {
            let fd = event.data as i32;

            if fd == listener.as_raw_fd() {
                // try accepting a connection
                match listener.accept() {
                    Ok((connection, _)) => {
                        connection.set_nonblocking(true).unwrap();

                        let fd = connection.as_raw_fd();

                        // register the connection with epoll
                        let event = Event::new(Events::EPOLLIN | Events::EPOLLOUT, fd as _);
                        epoll::ctl(epoll, EPOLL_CTL_ADD, fd, event).unwrap();

                        let state = ConnectionState::Read {
                            request: vec![0; 1024],
                            read: 0,
                        };

                        connections.insert(fd, (connection, state));
                    }
                    Err(e) if e.kind() == io::ErrorKind::WouldBlock => {}
                    Err(err) => panic!("failed to accept: {err}"),
                }

                continue 'next;
            }

            // otherwise, a connection must be ready
            let (connection, state) = connections.get_mut(&fd).unwrap();
            match state {
                ConnectionState::Read { request, read } => {
                    loop {
                        // try reading from the stream
                        match connection.read(&mut *request) {
                            Ok(0) => {
                                println!("client disconnected unexpectedly");
                                completed.push(fd);
                                continue 'next;
                            }
                            Ok(n) => {
                                *read += n;

                                if let Some(pos) =
                                    request.windows(4).position(|window| window == b"\r\n\r\n")
                                {
                                    // Parse the headers
                                    let headers_str = String::from_utf8_lossy(&request[..pos + 4]);
                                    let headers_vec: Vec<String> = headers_str
                                        .split("\r\n")
                                        .filter(|line| !line.is_empty())
                                        .map(|line| line.to_string())
                                        .collect();

                                    // Extract HTTP method, Content-Length, and Accept-Encoding
                                    let empty_string = String::new();
                                    let request_line = headers_vec.first().unwrap_or(&empty_string);
                                    let mut parts = request_line.split_whitespace();
                                    let method = parts.next().unwrap_or("").to_string();
                                    let endpoint = parts.next().unwrap_or("").to_string();

                                    let content_length = headers_vec
                                        .iter()
                                        .find(|line| line.starts_with("Content-Length:"))
                                        .and_then(|line| {
                                            line.split(':').nth(1).map(|s| s.to_string())
                                        })
                                        .and_then(|len| len.trim().parse::<usize>().ok())
                                        .unwrap_or(0);

                                    let accept_encoding = headers_vec
                                        .iter()
                                        .find(|line| line.starts_with("Accept-Encoding:"))
                                        .and_then(|line| {
                                            line.split(':').nth(1).map(|s| s.trim().to_string())
                                        });

                                    let content_type = headers_vec
                                        .iter()
                                        .find(|line| line.starts_with("Content-Type:"))
                                        .and_then(|line| {
                                            line.split(':').nth(1).map(|s| s.trim().to_string())
                                        });

                                    let context = RequestContext {
                                        method,
                                        endpoint,
                                        content_length,
                                        accept_encoding,
                                        content_type,
                                    };

                                    println!("Context: {:?}", context);

                                    // Store everything after \r\n\r\n
                                    let body_start = pos + 4;
                                    let body = request[body_start..*read].to_vec();
                                    let formatted_body = String::from_utf8_lossy(&body);
                                    println!("Body: {:?}", formatted_body);
                                    assert!(content_length == body.len());

                                    *state = ConnectionState::Process { context, body };
                                    break;
                                }
                            }
                            Err(err) if err.kind() == io::ErrorKind::WouldBlock => {
                                continue 'next;
                            }
                            Err(err) => panic!("{err}"),
                        }
                    }
                }

                ConnectionState::Process { context, body } => {
                    let response = if context
                        .content_type
                        .clone()
                        .unwrap_or("text/plain".to_owned())
                        == "application/json"
                        && context.endpoint == "/json"
                    {
                        let json_result: JsonResult<JsonBody> = serde_json::from_slice(&body);
                        match json_result {
                            Ok(mut json_body) => {
                                json_body.id += 1;
                                let modified_body =
                                    serde_json::to_string(&json_body).unwrap_or("".to_owned());
                                format!(
                                    "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
                                    modified_body.len(),
                                    modified_body
                                )
                            }
                            Err(e) => {
                                println!("Failed to parse JSON body: {}", e);
                                "HTTP/1.1 400 Bad Request\r\n\r\n".to_string()
                            }
                        }
                    } else {
                        concat!(
                            "HTTP/1.1 200 OK\r\n",
                            "Content-Length: 12\n",
                            "Connection: close\r\n\r\n",
                            "Hello world!"
                        )
                        .to_string()
                    };

                    *state = ConnectionState::Write {
                        response: response.as_bytes().to_vec(),
                        written: 0,
                    };
                }

                ConnectionState::Write { response, written } => {
                    loop {
                        match connection.write(&response[*written..]) {
                            Ok(0) => {
                                completed.push(fd);
                                continue 'next;
                            }
                            Ok(n) => {
                                *written += n;
                            }
                            Err(e) if e.kind() == io::ErrorKind::WouldBlock => {
                                // not ready yet, move on to the next connection
                                continue 'next;
                            }
                            Err(e) => panic!("{e}"),
                        }

                        // did we write the whole response yet?
                        if *written == response.len() {
                            break;
                        }
                    }

                    // successfully wrote the response, try flushing next
                    *state = ConnectionState::Flush;
                }

                ConnectionState::Flush => {
                    match connection.flush() {
                        Ok(_) => {
                            completed.push(fd);
                        }
                        Err(e) if e.kind() == io::ErrorKind::WouldBlock => {
                            // not ready yet, move on to the next connection
                            continue 'next;
                        }
                        Err(e) => panic!("{e}"),
                    }
                }
            }
        }

        //cleanup here
        for fd in completed {
            let (connection, _state) = connections.remove(&fd).unwrap();
            // unregister from epoll
            drop(connection);
        }
    }
}
