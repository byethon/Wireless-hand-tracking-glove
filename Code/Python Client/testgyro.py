import asyncio
import memcache
import websockets

server_address='127.0.0.1:11211'
server = memcache.Client(['127.0.0.1:11211'], debug=0)
server.set('output','Memcache server established')
async def getinfo():
    uri = "ws://192.168.4.1/ws"
    try:
        async with websockets.connect(uri) as websocket:
            while True:
                response = await websocket.recv()
                print(response)
                server.set('output',response)
    except:
        response="Server Not Found!"

while True:
    asyncio.get_event_loop().run_until_complete(getinfo())
