use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::time::Duration;

pub fn start() -> std::io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:8765")?;
    std::thread::spawn(move || {
        let clients = Arc::new(AtomicUsize::new(0));
        for stream in listener.incoming().flatten() {
            if clients.fetch_add(1, Ordering::AcqRel) >= 8 {
                clients.fetch_sub(1, Ordering::AcqRel);
                // Drop excess sockets immediately; never block the accept thread.
                continue;
            }
            let clients = clients.clone();
            std::thread::spawn(move || {
                let _ = serve(stream);
                clients.fetch_sub(1, Ordering::AcqRel);
            });
        }
    });
    Ok(())
}

fn serve(mut stream: TcpStream) -> std::io::Result<()> {
    stream.set_read_timeout(Some(Duration::from_secs(2)))?;
    stream.set_write_timeout(Some(Duration::from_secs(2)))?;
    let mut request = Vec::new();
    let mut buffer = [0; 1024];
    while !request.windows(4).any(|w| w == b"\r\n\r\n") {
        let n = stream.read(&mut buffer)?;
        if n == 0 || request.len() + n > 8192 {
            return Ok(());
        }
        request.extend_from_slice(&buffer[..n]);
    }
    let request = String::from_utf8_lossy(&request);
    let mut lines = request.lines();
    let first = lines.next().unwrap_or("");
    let host_valid = lines.any(|line| {
        line.split_once(':').is_some_and(|(key, value)| {
            key.eq_ignore_ascii_case("host") && value.trim() == "127.0.0.1:8765"
        })
    });
    if !host_valid {
        return response(
            &mut stream,
            "403 Forbidden",
            "text/plain",
            b"Loopback Host required",
        );
    }
    match first {
        "GET /telemetry HTTP/1.1" => {
            let data = super::UI_DATA.get().and_then(|d| d.lock().ok());
            let body = data
                .map(|d| {
                    format!(
                        "{} {} {}",
                        d.bpm,
                        u8::from(d.live),
                        u8::from(std::env::args().any(|a| a == "--demo"))
                    )
                })
                .unwrap_or_else(|| "0 0 0".into());
            response(&mut stream, "200 OK", "text/plain", body.as_bytes())
        }
        "GET /overlay HTTP/1.1" | "GET / HTTP/1.1" => response(
            &mut stream,
            "200 OK",
            "text/html; charset=utf-8",
            include_bytes!("overlay.html"),
        ),
        "GET /events HTTP/1.1" => {
            stream.write_all(b"HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n")?;
            loop {
                let snapshot = super::UI_DATA.get().and_then(|d| d.lock().ok()).map(|d| {
                    format!(
                        "data: {{\"bpm\":{},\"live\":{},\"demo\":{},\"status\":{}}}\n\n",
                        d.bpm,
                        d.live,
                        std::env::args().any(|a| a == "--demo"),
                        json_string(&d.status)
                    )
                });
                if let Some(snapshot) = snapshot {
                    stream.write_all(snapshot.as_bytes())?;
                    stream.flush()?;
                }
                std::thread::sleep(Duration::from_secs(1));
            }
        }
        _ => response(&mut stream, "404 Not Found", "text/plain", b"Not found"),
    }
}

fn response(stream: &mut TcpStream, status: &str, mime: &str, body: &[u8]) -> std::io::Result<()> {
    write!(stream, "HTTP/1.1 {status}\r\nContent-Type: {mime}\r\nContent-Length: {}\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n", body.len())?;
    stream.write_all(body)
}

fn json_string(text: &str) -> String {
    let mut result = String::from("\"");
    for c in text.chars() {
        match c {
            '"' => result.push_str("\\\""),
            '\\' => result.push_str("\\\\"),
            c if c < ' ' => result.push_str(&format!("\\u{:04x}", c as u32)),
            c => result.push(c),
        }
    }
    result.push('"');
    result
}

#[cfg(test)]
mod tests {
    #[test]
    fn status_json_escapes_controls_and_quotes() {
        assert_eq!(super::json_string("a\n\"\\"), "\"a\\u000a\\\"\\\\\"");
    }
}
