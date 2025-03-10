use crate::connection_handler::handle_connection;
use crate::thread_pool::ThreadPool;
use libc::{EPOLL_CTL_ADD, EPOLLIN, epoll_create1, epoll_ctl, epoll_event, epoll_wait};
use std::{
    io,
    net::{TcpListener, TcpStream},
    os::unix::io::{AsRawFd, FromRawFd},
    thread,
};

pub fn use_threads(listener: TcpListener) {
    for stream in listener.incoming() {
        let stream = stream.unwrap();

        thread::spawn(|| {
            handle_connection(stream);
        });
    }
}

pub fn use_thread_pool(listener: TcpListener) {
    let pool = ThreadPool::new(2);
    for stream in listener.incoming() {
        let stream = stream.unwrap();

        pool.execute(|| {
            handle_connection(stream);
        });
    }
}

const MAX_EVENTS: usize = 1024;
pub fn use_epoll(listener: TcpListener) -> io::Result<()> {
    unsafe {
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
                                    events: EPOLLIN as u32, //Edge triggered causes errors
                                    u64: conn_fd as u64,
                                };

                                if epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &mut event) == -1 {
                                    println!("Failed to add client socket to epoll");
                                    libc::close(conn_fd);
                                    continue;
                                }
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
                    let stream = TcpStream::from_raw_fd(fd);
                    stream.set_nonblocking(true)?;
                    handle_connection(stream);
                    libc::close(fd);
                }
            }
        }
    }
}
