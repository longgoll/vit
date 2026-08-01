use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;

static RESPONSE: &[u8] = b"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 27\r\nConnection: keep-alive\r\n\r\n{\"message\":\"Hello, World!\"}";

fn handle_client(mut stream: TcpStream) {
    let mut buffer = [0; 2048];
    loop {
        match stream.read(&mut buffer) {
            Ok(0) => break,
            Ok(_) => {
                if stream.write_all(RESPONSE).is_err() {
                    break;
                }
            }
            Err(_) => break,
        }
    }
}

fn main() {
    let listener = TcpListener::bind("0.0.0.0:8082").expect("Failed to bind port 8082");
    println!("Rust Server listening on port 8082...");
    for stream in listener.incoming() {
        if let Ok(stream) = stream {
            thread::spawn(move || handle_client(stream));
        }
    }
}
