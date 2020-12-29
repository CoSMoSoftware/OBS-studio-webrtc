package io.cosmosoftware.kite.obs.steps;

import static org.webrtc.kite.Utils.getStackTrace;
import static org.webrtc.kite.stats.StatsUtils.buildStatSummary;
import static org.webrtc.kite.stats.StatsUtils.getPCStatOvertime;
import static org.webrtc.kite.stats.StatsUtils.transformToJson;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.OBSViewerPage;
import io.cosmosoftware.kite.pages.BasePage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestStep;
import java.util.List;
import javax.json.JsonObject;
import org.webrtc.kite.stats.rtc.RTCStatList;
import org.webrtc.kite.stats.rtc.RTCStatMap;

public class GetStatsStep extends TestStep {

  private final JsonObject getStatsConfig;
  private final BasePage page;

  public GetStatsStep(Runner runner, JsonObject getStatsConfig, BasePage page) {
    super(runner);
    this.setOptional(true);
    this.getStatsConfig = getStatsConfig;
    this.page = page;
  }

  @Override
  protected void step() throws KiteTestException {
    OBSViewerPage obsViewerPage = null;
    MainPage mainPage = null;

    try {
      if (page instanceof OBSViewerPage) {
        obsViewerPage = (OBSViewerPage) page;
        List<String> pcIds = obsViewerPage.getPeerConnectionList();
        RTCStatMap statsOverTime = getPCStatOvertime(webDriver, getStatsConfig);
        for (int index = 0; index < getStatsConfig.getJsonArray("peerConnections").size();
            index++) {
          RTCStatList pcStats = statsOverTime.get(index);
          reporter
              .jsonAttachment(this.report, pcIds.get(index) + " (Raw)", transformToJson(pcStats));
          reporter.jsonAttachment(this.report, pcIds.get(index) + " Summary",
              buildStatSummary(pcStats));
        }
      } else {
        mainPage = (MainPage) page;
        String stats = mainPage.getStats();
        reporter
            .textAttachment(this.report, "OBS stats (Raw)", stats, "plain");
      }
    } catch (Exception e) {
      logger.error(getStackTrace(e));
      throw new KiteTestException("Failed to getStats", Status.BROKEN, e);
    }
  }

  @Override
  public String stepDescription() {
    return page instanceof OBSViewerPage
        ? "Get stats from peer connection"
        : "Get stats from OBS UI element";
  }
}
