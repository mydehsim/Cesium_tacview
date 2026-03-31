/**
 * XML Builder Module
 * Track verilerini Tacview uyumlu XML formatına dönüştürür
 */

export class XmlBuilder {
  /**
   * Track listesini XML formatına çevir
   * @param {Array} tracks - Track objeleri
   * @returns {string} XML string
   */
  static buildTracksXml(tracks) {
    let xml = '<?xml version="1.0" encoding="utf-8"?>\n';
    xml += "<Tracks>\n";

    if (Array.isArray(tracks) && tracks.length > 0) {
      tracks.forEach((track) => {
        xml += this.buildTrackElement(track);
      });
    }

    xml += "</Tracks>";
    return xml;
  }

  /**
   * Tek Track element'i oluştur
   */
  static buildTrackElement(track) {
    const {
      number = 0,
      callSign = "",
      type = "Fighter",
      color = "FF0000",
      engagementRange = 0,
      verticalEngagementRange = 0,
      waypoints = [],
      isSelected = false,
    } = track;

    let xml = "  <Track>\n";
    xml += this.buildElement("Number", number);
    xml += this.buildElement("CallSign", callSign);
    xml += this.buildElement("Type", type);
    xml += this.buildElement("Color", color);
    xml += this.buildElement("EngagementRange", engagementRange);
    xml += this.buildElement("VerticalEngagementRange", verticalEngagementRange);
    xml += this.buildElement("IsSelected", isSelected ? "true" : "false");

    if (Array.isArray(waypoints) && waypoints.length > 0) {
      xml += "    <Waypoints>\n";
      waypoints.forEach((waypoint) => {
        xml += this.buildWaypointElement(waypoint);
      });
      xml += "    </Waypoints>\n";
    }

    xml += "  </Track>\n";
    return xml;
  }

  /**
   * Waypoint element'i oluştur
   */
  static buildWaypointElement(waypoint) {
    const {
      latitude = 0,
      longitude = 0,
      altitude = 0,
      time = 0,
    } = waypoint;

    let xml = "      <Waypoint>\n";
    xml += this.buildElement("Latitude", latitude, 8); // 8 boşluk indent
    xml += this.buildElement("Longitude", longitude, 8);
    xml += this.buildElement("Altitude", altitude, 8);
    xml += this.buildElement("Time", time, 8);
    xml += "      </Waypoint>\n";
    return xml;
  }

  /**
   * Basit XML element oluştur
   */
  static buildElement(name, value, indent = 4) {
    const spaces = " ".repeat(indent);
    return `${spaces}<${name}>${this.escapeXml(value)}</${name}>\n`;
  }

  /**
   * XML özel karakterleri escape et
   */
  static escapeXml(str) {
    const replacements = {
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      '"': "&quot;",
      "'": "&apos;",
    };
    return String(str).replace(/[&<>"']/g, (char) => replacements[char]);
  }

  /**
   * Reset komutu XML'i oluştur
   */
  static buildResetXml() {
    let xml = '<?xml version="1.0" encoding="utf-8"?>\n';
    xml += '<Command type="RESET">\n';
    xml += '  <Action>CLEAR_TELEMETRY</Action>\n';
    xml += "</Command>";
    return xml;
  }

  /**
   * Command wrapper ile XML'i sar
   */
  static wrapCommand(xmlContent, commandType = "INJECT") {
    let xml = '<?xml version="1.0" encoding="utf-8"?>\n';
    xml += `<Command type="${commandType}">\n`;
    xml += xmlContent
      .split("\n")
      .slice(1) // XML header'ı atla
      .map((line) => (line ? "  " + line : line))
      .join("\n");
    xml += "\n</Command>";
    return xml;
  }
}

export default XmlBuilder;
