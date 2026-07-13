import asyncio
from bleak import BleakScanner, BleakClient

DEVICE_NAME = "MAINS_MCU"
CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a9"

def notification_handler(characteristic, data):
    try:
        print("DATA :", data.decode())
    except:
        print("RAW :", data)

async def main():

    print("Scanning...")

    devices = await BleakScanner.discover(timeout=5)

    target = None

    for d in devices:
        if d.name == DEVICE_NAME:
            target = d
            break

    if target is None:
        print("Device tidak ditemukan.")
        return

    async with BleakClient(target.address) as client:

        print("Connected")

        await client.start_notify(CHAR_UUID, notification_handler)

        print("Notify aktif...")
        print("Menunggu data...\n")

        await asyncio.sleep(30)

        await client.stop_notify(CHAR_UUID)

asyncio.run(main())