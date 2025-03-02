use hyper::header::CONTENT_TYPE;
use hyper::server::conn::Http;
use hyper::service::service_fn;
use hyper::{Body, Method, Request, Response, StatusCode};
use serde::{Deserialize, Serialize};
use serde_json::json;
use std::convert::Infallible;
use std::net::SocketAddr;
use tokio::net::TcpListener;

#[derive(Deserialize, Serialize, Debug)]
struct JsonBody {
    id: u32,
    value: String,
}

async fn handle_request(req: Request<Body>) -> Result<Response<Body>, Infallible> {
    let path = req.uri().path().to_string();
    let method = req.method().clone();

    if method == Method::GET && path.starts_with("/echo/") {
        let name = path.trim_start_matches("/echo/").to_string();
        Ok(Response::new(Body::from(name)))
    } else if method == Method::POST && path == "/json" {
        let whole_body = hyper::body::to_bytes(req.into_body()).await.unwrap();
        let json_body: Result<JsonBody, _> = serde_json::from_slice(&whole_body);

        match json_body {
            Ok(mut body) => {
                body.id += 1;
                let modified_body = serde_json::to_string(&body).unwrap();
                let mut response = Response::new(Body::from(modified_body));
                response
                    .headers_mut()
                    .insert(CONTENT_TYPE, "application/json".parse().unwrap());
                Ok(response)
            }
            Err(_) => {
                let mut response = Response::new(Body::from("Invalid JSON"));
                *response.status_mut() = StatusCode::BAD_REQUEST;
                response
                    .headers_mut()
                    .insert(CONTENT_TYPE, "application/json".parse().unwrap());
                Ok(response)
            }
        }
    } else {
        Ok(Response::new(Body::from("Hello, World!")))
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
    let addr = SocketAddr::from(([0, 0, 0, 0], 4221));

    let listener = TcpListener::bind(addr).await?;
    println!("Listening on http://{}", addr);
    loop {
        let (tcp, _) = listener.accept().await?;
        let io = tcp;

        tokio::task::spawn(async move {
            if let Err(err) = Http::new()
                .serve_connection(io, service_fn(handle_request))
                .await
            {
                println!("Error serving connection: {:?}", err);
            }
        });
    }
}
