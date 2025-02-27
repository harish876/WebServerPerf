use crate::connection_handler::HandleConnection;
use crate::thread_pool::ThreadPool;
use libc::{EPOLL_CTL_ADD, EPOLLET, EPOLLIN, epoll_create1, epoll_ctl, epoll_event, epoll_wait};
use std::{
    collections::HashMap,
    io,
    net::{TcpListener, TcpStream},
    os::fd::{AsRawFd, RawFd},
    thread,
};

pub fn use_threads(listener: TcpListener) {
    for stream in listener.incoming() {
        let stream = stream.unwrap();

        thread::spawn(|| {
            stream.handle_connection();
        });
    }
}

pub fn use_thread_pool(listener: TcpListener) {
    let pool = ThreadPool::new(100);
    for stream in listener.incoming() {
        let stream = stream.unwrap();

        pool.execute(|| {
            stream.handle_connection();
        });
    }
}

const MAX_EVENTS: usize = 1024;
pub fn use_epoll(listener: TcpListener) -> io::Result<()> {
    unsafe {
        // Set the listener to non-blocking mode
        listener.set_nonblocking(true)?;

        let epoll_fd = epoll_create1(0);
        if epoll_fd == -1 {
            return Err(io::Error::last_os_error());
        }

        let listener_fd = listener.as_raw_fd();

        let mut event = epoll_event {
            events: EPOLLIN as u32,
            u64: listener_fd as u64,
        };

        if epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener_fd, &mut event) == -1 {
            return Err(io::Error::last_os_error());
        }

        let mut events = vec![epoll_event { events: 0, u64: 0 }; MAX_EVENTS];
        let mut clients: HashMap<RawFd, TcpStream> = HashMap::new();

        loop {
            let n = epoll_wait(epoll_fd, events.as_mut_ptr(), MAX_EVENTS as i32, -1);
            if n == -1 {
                println!("Failed to wait on fd set");
                libc::close(epoll_fd);
                return Err(io::Error::last_os_error());
            }

            for i in 0..n as usize {
                let fd = events[i].u64 as i32;

                if fd == listener_fd {
                    loop {
                        match listener.accept() {
                            Ok((stream, _)) => {
                                let conn_fd = stream.as_raw_fd();
                                stream.set_nonblocking(true)?;

                                let mut event = epoll_event {
                                    events: EPOLLIN as u32 | EPOLLET as u32,
                                    u64: conn_fd as u64,
                                };

                                if epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &mut event) == -1 {
                                    println!("Failed to add client socket to epoll");
                                    libc::close(conn_fd);
                                    continue;
                                }

                                clients.insert(conn_fd, stream);
                            }
                            Err(ref e) if e.kind() == io::ErrorKind::WouldBlock => {
                                break;
                            }
                            Err(e) => {
                                println!("Failed to accept connection: {}", e);
                                return Err(e);
                            }
                        }
                    }
                } else {
                    if let Some(stream) = clients.remove(&fd) {
                        stream.handle_connection();
                        libc::close(fd);
                    }
                }
            }
        }
    }
}
