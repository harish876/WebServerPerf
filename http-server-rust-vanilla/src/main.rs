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
        let thread_pool_size = if mode == "thread_pool" {
            args.get(2)
                .and_then(|s| s.parse::<usize>().ok())
                .unwrap_or(2) // Default pool size if not specified or invalid
        } else {
            2 // Default value for other modes (though not used)
        };
        server::use_thread_pool(listener.try_clone().unwrap(), thread_pool_size);
    } else if mode == "epoll" {
        listener.set_nonblocking(true)?;
        let _ = server::use_epoll(listener.try_clone().unwrap());
    } else {
        panic!("invalid arg...")
    }

    Ok(())
}
