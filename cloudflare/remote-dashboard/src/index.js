import { DurableObject } from "cloudflare:workers";

export class BatteryRelay extends DurableObject {
  async fetch(request) {
    const upgrade = request.headers.get("Upgrade");

    if (!upgrade || upgrade.toLowerCase() !== "websocket") {
      return new Response("Expected WebSocket", {
        status: 426,
      });
    }

    const pair = new WebSocketPair();
    const [client, server] = Object.values(pair);

    const url = new URL(request.url);
    const role = url.pathname === "/browser" ? "browser" : "device";

    this.ctx.acceptWebSocket(server, [role]);

    return new Response(null, {
      status: 101,
      webSocket: client,
    });
  }

  webSocketMessage(webSocket, message) {
    const tags = this.ctx.getTags(webSocket);

    if (tags.includes("device")) {
      for (const client of this.ctx.getWebSockets("browser")) {
        client.send(message);
      }
      return;
    }

    if (tags.includes("browser")) {
      for (const device of this.ctx.getWebSockets("device")) {
        device.send(message);
      }
    }
  }
}
export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    if (url.pathname === "/device") {
      const authorization = request.headers.get("Authorization");

      if (authorization !== `Bearer ${env.DEVICE_TOKEN}`) {
        return new Response("Unauthorized", {
          status: 401,
        });
      }

      const relay = env.BATTERY_RELAY.getByName("battery");
      return relay.fetch(request);
    }
    if (url.pathname === "/browser") {
      const relay = env.BATTERY_RELAY.getByName("battery");
      return relay.fetch(request);
    }
    return new Response("Not found", {
      status: 404,
    });
  },
};
