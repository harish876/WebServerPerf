use http_server_rust::server;
use std::{env, net::TcpListener};

fn main() -> std::io::Result<()> {
    let args: Vec<String> = env::args().collect();
    let binding = "threads".to_string();
    let mode = args.get(1).unwrap_or(&binding);
    let listener = TcpListener::bind("0.0.0.0:4221").unwrap();

    println!("Running server with mode: {}", mode);

    if mode == "threads" {
        server::use_threads(listener.try_clone().unwrap());
    } else if mode == "thread_pool" {
        server::use_thread_pool(listener.try_clone().unwrap());
    } else if mode == "epoll" {
        listener.set_nonblocking(true)?;
        let _ = server::use_epoll(listener.try_clone().unwrap());
    } else {
        panic!("invalid arg...")
    }

    Ok(())
}
