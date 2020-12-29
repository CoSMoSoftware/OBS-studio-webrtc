package io.cosmosoftware.kite.obs.pages;

import static io.cosmosoftware.kite.entities.Timeouts.FIVE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.THREE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.util.ReportUtils.getStackTrace;
import static io.cosmosoftware.kite.util.ReportUtils.timestamp;
import static io.cosmosoftware.kite.util.TestUtils.executeJsScript;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;

import io.cosmosoftware.kite.exception.KiteInteractionException;
import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.pages.BasePage;
import io.cosmosoftware.kite.report.Status;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.FindBy;

public class OBSViewerPage extends BasePage {

  @FindBy(tagName = "video")
  private List<WebElement> videos;

  // there are more than 1
  @FindBy(className = "room-card-overlay-button")
  private List<WebElement> roomCards;

  // there are more than 1
  @FindBy(id = "displayName")
  private List<WebElement> displayNames;

  @FindBy(id = "email")
  private WebElement email;

  @FindBy(id = "password")
  private WebElement password;

  @FindBy(id = "stream-video")
  private WebElement streamvideo;

  // only one with this class name so far
  @FindBy(className = "jss92")
  private WebElement logInButton;
  // only one with this class name so far
  @FindBy(className = "pre-room-control-bar-go-live-button")
  private WebElement goLiveBtn;
  // only one with this class name so far
  @FindBy(className = "room-settings-streamkey-show-button")
  private WebElement showStreamKeyBtn;
  // only one with this class name so far
  @FindBy(className = "room-settings-streamkey-create-button")
  private WebElement createStreamKeyBtn;

  @FindBy(id = "dialog-cancel-button")
  private WebElement closeDialogBtn;

  @FindBy(className = "icon-button-icon")
  private List<WebElement> iconButtons;

  @FindBy(className = "obs-info-value")
  private List<WebElement> obsInfos;

  @FindBy(className = "browser-not-supported-title")
  private List<WebElement> browserNotSupported;

  private final int noOfParticipants;
  private final String pcPrefix;

  public OBSViewerPage(Runner runner, int noOfParticipants, String pcPrefix) {
    super(runner);
    this.noOfParticipants = noOfParticipants;
    this.pcPrefix = pcPrefix;
  }

  public List<WebElement> getVideos() {
    return videos;
  }

  /**
   * Open page to the room
   *
   * @param url the url to the streaming room
   */
  public void openPage(String url) throws KiteTestException {
    //loadPage(this.webDriver, url, TEN_SECOND_INTERVAL); // might have problem
    this.webDriver.get(url); // use this in case load page does not work
    waitAround(THREE_SECOND_INTERVAL);
    if (!browserNotSupported.isEmpty()) {
      throw new KiteTestException("Browser is not currently supported", Status.FAILED);
    }
  }

  /**
   * Login to the room with a user name (timestamp)
   */
  public void logInIfNecessary() throws KiteInteractionException {
    if (displayNames.size() > 1) {
      sendKeys(displayNames.get(1), timestamp() + "\n");
      waitAround(FIVE_SECOND_INTERVAL);
    }
  }

  public void goLive() throws KiteInteractionException {
    clickWithJs("pre-room-control-bar-go-live-button", 0);
    waitAround(FIVE_SECOND_INTERVAL);
  }

  public void openOBSSettings() throws KiteInteractionException {
    if (!iconButtons.get(5).isDisplayed()) {
      logger.info("Icon not showing, click screen");
      click(streamvideo);
      waitAround(ONE_SECOND_INTERVAL);
    }
    clickWithJs("icon-button", 5);

    waitAround(FIVE_SECOND_INTERVAL);

  }

  public void closeOBSSettings() throws KiteInteractionException {
    clickWithJsById("dialog-cancel-button");
  }

  public void generateKeyIfNecessary() throws KiteInteractionException {
    if (showStreamKeyBtn != null) {
      clickWithJs("room-settings-streamkey-show-button", 0);

      waitAround(ONE_SECOND_INTERVAL);
      if (createStreamKeyBtn != null) {
        clickWithJs("room-settings-streamkey-create-button", 0);

        waitAround(ONE_SECOND_INTERVAL);
      }
    }
  }

  public String getServerName() {
    return obsInfos.get(2).getText();
  }

  public String getServerRoom() {
    return obsInfos.get(3).getText();
  }

  public String getServerKey() {
    return obsInfos.get(4).getText();
  }

  public String getSdpMessage(String remoteOrLocal) throws KiteInteractionException {
    String id = this.getOBSPeerConnectionObject();
    try {
      return (String) executeJsScript(webDriver, "return " + id + ".current"
          + remoteOrLocal
          + "Description.sdp");
    } catch (Exception e) {
      throw new KiteInteractionException(
          "Could not get the sdp from " + id + " :" + e.getLocalizedMessage());
    }
  }

  public List<String> getPeerConnectionList()
      throws KiteInteractionException {
    List<String> res = new ArrayList<>();
    try {
      for (int index = 0; index < this.noOfParticipants; index++) {
        String pc = (String) executeJsScript(webDriver,
            "return " + pcPrefix + "[" + index + "]" + "[0]");
        res.add(pc);
      }
    } catch (Exception e) {
      throw new KiteInteractionException(
          "Could not get the peer connection id list: " + e.getLocalizedMessage());
    }
    return res;
  }

  public String getOBSPeerConnectionObject() throws KiteInteractionException {
    String res = null;
    List<String> pcIds = this.getPeerConnectionList();
    for (int index = 0; index < pcIds.size(); index++) {
      String pcId = pcIds.get(index);
      if (pcId.startsWith("OBS")) {
        res = this.pcPrefix + "[" + index + "][1]";
      }
    }
    return res;
  }

  public int getNoOfParticipants() {
    return noOfParticipants;
  }

  public void inputEmail (String emailString) throws KiteInteractionException {
    sendKeys(email, emailString);
  }
  public void inputPassword (String passwordString) throws KiteInteractionException {
    sendKeys(password, passwordString);
  }

  public void logIn() throws KiteInteractionException {
    clickWithJs("jss92",0);
  }

  public void chooseRoomCard() throws KiteInteractionException {
    click(roomCards.get(1));
  }

  public void dismissPopup() throws KiteInteractionException {
    if (!webDriver.findElements(By.id("dialog-cancel-button")).isEmpty()) {
      clickWithJsById("dialog-cancel-button");
    }
  }



  private void clickWithJs(String className, int index) throws KiteInteractionException {
    try {
      executeJsScript(webDriver,
          "document.getElementsByClassName('" + className + "')[" + index + "].click()");
    } catch (Exception e) {
      throw new KiteInteractionException("Could not execute clicking by JavaScript: ");
    }
  }

  private void clickWithJsById(String id) throws KiteInteractionException {
    try {
      executeJsScript(webDriver, "document.getElementById('" + id + "').click()");
    } catch (Exception e) {
      throw new KiteInteractionException("Could not execute clicking by JavaScript: ");
    }
  }

  public boolean peerConnectionExists() {
    try {
      return getOBSPeerConnectionObject() != null;
    } catch (Exception e) {
      logger.info("There was exception while getting the peer connection: ");
      logger.info(getStackTrace(e));
      return false;
    }
  }
}
