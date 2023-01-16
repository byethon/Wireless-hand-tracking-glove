import asyncio
import websockets

async def hello():
    uri = "ws://192.168.4.1/ws"
    async with websockets.connect(uri) as websocket:
            await websocket.send(input())

asyncio.get_event_loop().run_until_complete(hello())