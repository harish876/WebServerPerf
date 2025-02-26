use hyper::service::{make_service_fn, service_fn};
use hyper::{Body, Request, Response, Server};
use std::convert::Infallible;
use std::net::SocketAddr;

async fn handle_request(req: Request<Body>) -> Result<Response<Body>, Infallible> {
    let path = req.uri().path().to_string();
    if path.starts_with("/echo/") {
        let name = path.trim_start_matches("/echo/").to_string();
        Ok(Response::new(Body::from(name)))
    } else {
        Ok(Response::new(Body::from("Hello, World!")))
    }
}

#[tokio::main]
async fn main() {
    // Define the address to bind the server to
    let addr = SocketAddr::from(([0, 0, 0, 0], 4221));

    // Create a service that handles incoming requests
    let make_svc =
        make_service_fn(|_conn| async { Ok::<_, Infallible>(service_fn(handle_request)) });

    // Create the server
    let server = Server::bind(&addr).serve(make_svc);

    // Run the server
    println!("Listening on http://{}", addr);
    if let Err(e) = server.await {
        eprintln!("server error: {}", e);
    }
}
