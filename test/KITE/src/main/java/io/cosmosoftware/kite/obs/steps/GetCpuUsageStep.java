package io.cosmosoftware.kite.obs.steps;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestStep;
import java.net.URL;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import org.webrtc.kite.tests.TestRunner;

public class GetCpuUsageStep extends TestStep {

  private final TestRunner runner;
  private final MainPage mainPage;
  private double usage = 0.0;
  private List<Double> usages = new ArrayList<>();

  public GetCpuUsageStep(Runner runner) {
    super(runner);
    this.runner = (TestRunner) runner;
    this.mainPage = new MainPage(runner);
    this.setOptional(true);

  }

  @Override
  protected void step() throws KiteTestException {
    // expecting template: http://192.168.1.203:5001
    if (this.runner.getSessionData().containsKey("node_host")) {
      try {
        String nodeHost = (String) this.runner.getSessionData().get("node_host");
        String nodeIp = new URL(nodeHost).getHost();
        usages = mainPage.getCpuUsages(5, nodeIp + ":8080");
        // usage = mainPage.getCpuUsage(5, nodeIp + ":8080");
        this.report.setName(this.stepDescription());
        reporter.textAttachment(this.report, "Cpu Usage", usages + "", "plain");
//        logger.info("Cpu Monitoring: " + usage);
      } catch (Exception e) {
        throw new KiteTestException("Could not get the cpu usage: " + e.getLocalizedMessage(),
            Status.FAILED);
      }
    } else {
      throw new KiteTestException("Could not get node Ip to send command from session data",
          Status.FAILED);
    }

  }

  private double getAverage(List<Double> values) {
    if (values.isEmpty()) {
      return 0.0;
    }
    double res = 0.0;
    for (double value : values) {
      res += value;
    }
    return res / values.size();
  }

  @Override
  public String stepDescription() {
    return "Retrieving cpu usage (in %) from node's system: "
        + (getAverage(this.usages) == 0 ? "N/A"
        : new DecimalFormat("##.##").format(getAverage(this.usages)));
  }
}
