//! passviewer-fetch: one HTTPS request for the MeeGo edition of Pass Viewer.
//!
//!     passviewer-fetch <url> <authentication token> <output file> [if-modified-since]
//!
//! Sends the GET the Sailfish version sends from QNetworkAccessManager
//! (Authorization: ApplePass <token>, If-Modified-Since when known), writes
//! the body to the output file and reports one JSON object on stdout:
//!
//!     {"status": 200}            the body is in the file
//!     {"status": 304}            not modified, nothing written
//!     {"error": "..."}           no answer at all (DNS, TLS, timeout)
//!
//! The exit code is always 0 once the arguments are right; the caller reads
//! the JSON. Built statically against musl, so Harmattan's glibc 2.10 and
//! its OpenSSL 0.9.8 play no part.
use std::env;
use std::fs;
use std::io::Write;
use std::process;
use std::time::Duration;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 4 {
        eprintln!("usage: passviewer-fetch <url> <token> <output file> [if-modified-since]");
        process::exit(2);
    }
    let url = &args[1];
    let token = &args[2];
    let output = &args[3];
    let since = args.get(4).filter(|s| !s.is_empty());

    let result = fetch(url, token, since.map(|s| s.as_str()));
    let report = match result {
        Ok((status, body)) => {
            if status == 200 {
                if let Err(e) = fs::write(output, &body) {
                    serde_json::json!({ "error": format!("cannot write {}: {}", output, e) })
                } else {
                    serde_json::json!({ "status": status, "bytes": body.len() })
                }
            } else {
                serde_json::json!({ "status": status })
            }
        }
        Err(e) => serde_json::json!({ "error": e }),
    };
    let mut out = std::io::stdout();
    let _ = writeln!(out, "{}", report);
}

fn fetch(url: &str, token: &str, since: Option<&str>) -> Result<(u16, Vec<u8>), String> {
    let client = reqwest::blocking::Client::builder()
        .timeout(Duration::from_secs(40))
        .user_agent("harbour-passviewer/1.7 (MeeGo Harmattan)")
        .build()
        .map_err(|e| e.to_string())?;
    let mut request = client
        .get(url)
        .header("Authorization", format!("ApplePass {}", token));
    if let Some(since) = since {
        request = request.header("If-Modified-Since", since);
    }
    let response = request.send().map_err(|e| describe(&e))?;
    let status = response.status().as_u16();
    if status == 200 {
        let body = response.bytes().map_err(|e| describe(&e))?;
        Ok((status, body.to_vec()))
    } else {
        Ok((status, Vec::new()))
    }
}

// reqwest's Display stops at "error sending request"; the cause underneath
// is what tells DNS from TLS from a refused connection.
fn describe(e: &reqwest::Error) -> String {
    let mut text = e.to_string();
    let mut source = std::error::Error::source(e);
    while let Some(inner) = source {
        text.push_str(": ");
        text.push_str(&inner.to_string());
        source = inner.source();
    }
    text
}
