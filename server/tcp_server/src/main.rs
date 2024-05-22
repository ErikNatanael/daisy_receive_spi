//! A "hello world" echo server with Tokio
//!
//! This server will create a TCP listener, accept connections in a loop, and
//! write back everything that's read off of each TCP connection.
//!
//! Because the Tokio runtime uses a thread pool, each TCP connection is
//! processed concurrently with all other TCP connections across multiple
//! threads.
//!
//! To see this server in action, you can run this in one terminal:
//!
//!     cargo run --example echo
//!
//! and in another terminal you can run:
//!
//!     cargo run --example connect 127.0.0.1:8080
//!
//! Each line you type in to the `connect` terminal should be echo'd back to
//! you! If you open up multiple terminals running the `connect` example you
//! should be able to see them all make progress simultaneously.

#![warn(rust_2018_idioms)]

use tokio::fs::File;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpListener;

use anyhow::Result;
use core::num;
use std::env;
use std::error::Error;
use std::path::{Path, PathBuf};
use std::time::Duration;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    // Read sound file

    let paths = [
        "/home/erik/Musik/Focus_16bit_48000.wav",
        "/home/erik/Musik/Unknown_16bit_48000.wav",
    ];
    let sound_buffers: Vec<Vec<u8>> = paths
        .into_iter()
        .map(|path| open_wav_as_bytes(path))
        .collect::<Result<Vec<_>, _>>()?;

    for sb in &sound_buffers {
        println!("Audio buffer is {} bytes", sb.len());
    }
    // Create a 'static ref
    let sound_buffers = sound_buffers.leak();

    // Allow passing an address to listen on as the first argument of this
    // program, but otherwise we'll just set up our TCP listener on
    // 127.0.0.1:8080 for connections.
    let addr = env::args()
        .nth(1)
        .unwrap_or_else(|| "0.0.0.0:8081".to_string());

    // Next up we create a TCP listener which will listen for incoming
    // connections. This TCP listener is bound to the address we determined
    // above and must be associated with an event loop.
    let listener = TcpListener::bind(&addr).await?;
    println!("Listening on: {}", addr);

    loop {
        // Asynchronously wait for an inbound socket.
        let (mut socket, _) = listener.accept().await?;

        // And this is where much of the magic of this server happens. We
        // crucially want all clients to make progress concurrently, rather than
        // blocking one on completion of another. To achieve this we use the
        // `tokio::spawn` function to execute the work in the background.
        //
        // Essentially here we're executing a new task to run concurrently,
        // which will allow all of our clients to be processed concurrently.
        let sound_buffers = &*sound_buffers;
        tokio::spawn(async move {
            let mut buf = vec![0; 1024];
            println!("Connected");

            // In a loop, read data from the socket and write the data back.
            loop {
                let n = socket
                    .read(&mut buf)
                    .await
                    .expect("failed to read data from socket");

                if n == 0 {
                    println!("Client disconnected");
                    return;
                } else {
                    println!(
                        "Msg: {}",
                        &buf[0..n].iter().map(|b| *b as char).collect::<String>()
                    );
                }

                // socket
                //     .write_all(&buf[0..n])
                //     .await
                //     .expect("failed to write data to socket");
                loop {
                    for sb in sound_buffers {
                        socket
                            .write_all("audio\n".as_bytes())
                            .await
                            .expect("failed to write data to socket");
                        println!("Sent \"audio\"");
                        socket
                            .write_all(&(sb.len() as u32).to_le_bytes())
                            .await
                            .expect("failed to write data to socket");
                        println!("Sent length: {}", sb.len());
                        socket
                            .write_all(sb)
                            .await
                            .expect("failed to write data to socket");
                        println!("Sent data");
                        tokio::time::sleep(Duration::from_secs_f32(2.)).await;
                    }
                }
            }
        });
    }
}

fn open_wav_as_bytes(path: impl AsRef<Path>) -> Result<Vec<u8>> {
    let mut inp_file = std::fs::File::open(path)?;
    let (header, data) = wav::read(&mut inp_file)?;

    println!("Bytes per sample: {}", header.bytes_per_sample);
    let mut num_samples = None;
    let sound_bytes: Vec<u8> = match data {
        wav::BitDepth::Eight(_) => todo!(),
        wav::BitDepth::Sixteen(buf) => {
            num_samples = Some(buf.len());
            buf.into_iter().map(|v| v.to_le_bytes()).flatten().collect()
        }
        wav::BitDepth::TwentyFour(_) => todo!(),
        wav::BitDepth::ThirtyTwoFloat(_) => todo!(),
        wav::BitDepth::Empty => todo!(),
    };
    let num_samples = num_samples.unwrap();
    println!("samples: {}, bytes: {}", num_samples, sound_bytes.len());

    Ok(sound_bytes)
}
