package io.cosmosoftware.kite.obs.pages;

import static io.cosmosoftware.kite.entities.Timeouts.DEFAULT_TIMEOUT;
import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.THREE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;

import io.cosmosoftware.kite.exception.KiteInteractionException;
import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.elements.MainPageElements;
import io.cosmosoftware.kite.obs.pages.elements.PageElements;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.util.TestUtils;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.remote.RemoteWebDriver;

public class MainPage extends Page {

  private final MainPageElements mainPageElements = new MainPageElements();
  private boolean streaming = false;

  public MainPage(Runner runner) {
    super(runner);
    this.os = ((RemoteWebDriver) runner.getWebDriver()).getCapabilities().getPlatform().toString();
    this.elements = this.mainPageElements.populateElement(this.os);
  }

  public void startStreaming() throws KiteInteractionException {
    click(getElement(PageElements.MAIN_PAGE_START_STREAMING_BTN));
    confirmStreaming();
  }

  public String getStreamingButtonStatus() throws KiteInteractionException {
    WebElement element = getElement(PageElements.MAIN_PAGE_START_STREAMING_BTN);
    return this.os.equalsIgnoreCase("WINDOWS") ? element.getAttribute("Name")
        : element.getTagName();
  }

  public void stopStreaming() throws KiteInteractionException {
    click(getElement(PageElements.MAIN_PAGE_START_STREAMING_BTN));
    this.streaming = false;
  }

  public void quitApplication() throws KiteInteractionException {
    if (this.streaming) {
      stopStreaming();
    }
    click(getElement(PageElements.MAIN_PAGE_EXIT_BTN));
  }

  public void openSettings() throws KiteInteractionException {
    waitAround(THREE_SECOND_INTERVAL);
    click(getElement(PageElements.MAIN_PAGE_SETTING_BTN));
    waitAround(THREE_SECOND_INTERVAL);
  }

  public List<String> getButtonLabels() {
    List<WebElement> buttons;
    List<String> labels = new ArrayList<>();
    if (this.os.equals("WINDOWS")) {
      buttons = this.webDriver
          .findElements(By.xpath(this.elements.get(PageElements.MAIN_PAGE_BUTTON)));
    } else { // assuming linux works the same way as windows
      buttons = this.getElementByXpath(this.elements.get(PageElements.MAIN_PAGE_BUTTON));
    }

    for (WebElement button : buttons) {
      labels.add(this.os.equalsIgnoreCase("WINDOWS") ? button.getText()
          : button.getTagName()); // this does not work on MAC
    }
    return labels;
  }

  public List<String> getStatusTexts() {
    List<WebElement> buttons;
    List<String> labels = new ArrayList<>();
    if (this.os.equals("WINDOWS")) {
      buttons = this.webDriver
          .findElements(By.xpath(this.elements.get(PageElements.MAIN_PAGE_STATUS_TEXT)));
    } else { // assuming linux works the same way as windows
      buttons = this.getElementByXpath(this.elements.get(PageElements.MAIN_PAGE_STATUS_TEXT));
    }

    for (WebElement button : buttons) {
      labels.add(button.getText()); // this does not work on MAC
    }
    return labels;
  }

  private void confirmStreaming() throws KiteInteractionException {
    for (int wait = 0; wait < DEFAULT_TIMEOUT; wait += ONE_SECOND_INTERVAL) {
      // it will be larger if there are numbers, there's nothing if not streaming
      if (getElement(PageElements.MAIN_PAGE_LIVE_STATUS).getText().contains("00:00:00")) {
        waitAround(ONE_SECOND_INTERVAL);
      } else {
        waitAround(THREE_SECOND_INTERVAL); // just in case there's no video on chrome side yet
        this.streaming = true;
        return;
      }
    }
    throw new KiteInteractionException("Could not confirm streaming status after 60s");
  }

  public String getStats() throws KiteInteractionException {
    String stats =  /*this.os.equalsIgnoreCase("windows")
          ? getElement(PageElements.MAIN_PAGE_STATS).getAttribute("Name")
          :*/ getElement(PageElements.MAIN_PAGE_STATS).getText();
    return stats == null ? "Could not get stats from UI element" : stats;
  }

  public boolean crashed() {
    try {
      return !this.getElement(PageElements.MAIN_PAGE_LIVE_STATUS).isDisplayed();
    } catch (Exception e) {
      return false;
    }
  }

  public double getCpuUsage(int duration, String serverHost) throws KiteTestException {
    return getAverage(getCpuUsages(duration, serverHost));
  }

  public List<Double> getCpuUsages(int duration, String serverHost) throws KiteTestException {
    List<Double> cpuUsage = new ArrayList<>();
    String request = "http://" + serverHost + "/Grid/CpuMonitoring?process=" + getProcessName();
    for (int index = 0; index < duration; index++) {
      String response = TestUtils.doHttpGet(request);
      if (!response.contains("Exception")) {
        if (!response.equals("NA") && !response.equals("-1")) {
          cpuUsage.add(Double.parseDouble(response));
        }
      }
    }
    return cpuUsage;
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

  private String getProcessName() throws KiteTestException {
    switch (this.os) {
      case "WINDOWS":
        return "obs64";
      case "MAC":
        return "OBS";
      default:
        throw new KiteTestException("Unsupported OS", Status.BROKEN);
    }
  }

}
