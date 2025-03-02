# Navigate to the http-server directory
cd ../http-server-rust-vanilla

# Build the Rust HTTP server
echo "Building the Rust HTTP server..."
cargo build

# Check if the build was successful
if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

# Run the Rust HTTP server on CPU core 0
echo "Starting the Rust HTTP server on CPU core 0..."
```
sudo taskset -c 0 ./target/debug/http_server_rust epoll
```