use crate::connection_handler::{ConnectionStatus, handle_connection};
use crate::thread_pool::ThreadPool;
use libc::{EPOLL_CTL_ADD, EPOLLIN, close, epoll_create1, epoll_ctl, epoll_event, epoll_wait};
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

        thread::spawn(move || {
            handle_connection(&stream);
        });
    }
}

pub fn use_thread_pool(listener: TcpListener, thread_pool_size: usize) {
    let pool = ThreadPool::new(thread_pool_size);
    for stream in listener.incoming() {
        let stream = stream.unwrap();

        pool.execute(move || {
            handle_connection(&stream);
        });
    }
}

const MAX_EVENTS: usize = 1024;
pub fn use_epoll(listener: TcpListener) -> io::Result<()> {
    unsafe {
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

        'event_loop: loop {
            let n = epoll_wait(epoll_fd, events.as_mut_ptr(), MAX_EVENTS as i32, -1);
            if n == -1 {
                println!("Failed to wait on fd set");
                close(epoll_fd);
                return Err(io::Error::last_os_error());
            }

            for i in 0..n as usize {
                let fd = events[i].u64 as i32;

                if fd == listener_fd {
                    match listener.accept() {
                        Ok((stream, _)) => {
                            let conn_fd = stream.as_raw_fd();
                            stream.set_nonblocking(true)?;

                            let mut event = epoll_event {
                                events: EPOLLIN as u32,
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
                            continue;
                        }
                        Err(e) => {
                            println!("Failed to accept connection: {}", e);
                            break 'event_loop;
                        }
                    }
                } else {
                    if let Some(stream) = clients.get_mut(&fd) {
                        match handle_connection(stream) {
                            ConnectionStatus::Complete => {
                                clients.remove(&fd);
                                close(fd);
                            }
                            ConnectionStatus::WouldBlock => {
                                println!("Connection would block, keeping for later");
                                continue;
                            }
                            ConnectionStatus::Error => {
                                clients.remove(&fd);
                                close(fd);
                            }
                        }
                    }
                }
            }
        }
        close(epoll_fd);
        Ok(())
    }
}
