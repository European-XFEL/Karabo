import json

VALID_TOKEN = "01234567-89ab-cdef-0123-456789abcdef"
VALID_USER_ID = "Bob"
INVALID_TOKEN_MSG = "Invalid one-time token!"
VALID_ACCESS_LEVEL = 1


async def handle_client(reader, writer):
    data = await reader.read(1024)
    message = data.decode()
    if not message.startswith("POST"):
        response = (
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 39\r\n\r\n"
            "Unsupported method - only POST is supported."
        )
        writer.write(response.encode())
        await writer.drain()
        writer.close()
        return

    try:
        body = message.split("\r\n\r\n", 1)[1]
        payload = json.loads(body)
        token = payload.get("tk", "")
    except Exception:
        response = (
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n\r\n"
            "Bad Request"
        )
        writer.write(response.encode())
        await writer.drain()
        writer.close()
        return

    if token == VALID_TOKEN:
        res_data = {
            "success": True,
            "username": VALID_USER_ID,
            "visibility": VALID_ACCESS_LEVEL,
            "error_msg": ""
        }
    else:
        res_data = {
            "success": False,
            "username": "",
            "visibility": 0,
            "error_msg": INVALID_TOKEN_MSG
        }

    response_body = json.dumps(res_data).encode()
    response = (
        "HTTP/1.1 200 OK\r\n"
        f"Content-Type: application/json\r\n"
        f"Content-Length: {len(response_body)}\r\n"
        "\r\n"
    ).encode() + response_body

    writer.write(response)
    await writer.drain()
    writer.close()
