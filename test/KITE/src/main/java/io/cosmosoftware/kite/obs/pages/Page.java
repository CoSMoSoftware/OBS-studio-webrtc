package io.cosmosoftware.kite.obs.pages;

import io.cosmosoftware.kite.exception.KiteInteractionException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.pages.BasePage;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

public abstract class Page extends BasePage {

  //  protected HashMap<String, WebElement> elements;
  protected String os;
  protected HashMap<String, String> elements;

  protected Page(Runner runner) {
    super(runner);
  }

  protected WebElement getElement(String elementName) throws KiteInteractionException {
    try {
      return this.webDriver.findElement(By.xpath(this.elements.get(elementName)));
    } catch (Exception e) {
      logger.debug(e.getStackTrace());
      throw new KiteInteractionException("Could not find element: " + elementName);
    }
  }

  protected List<WebElement> getElementByXpath(String xpath) {
    List<WebElement> res = new ArrayList<>();
    int count = 0;
    boolean done = false;
    while (!done) {
      try {
        WebElement element = webDriver.findElement(By.xpath(xpath + "[" + count + "]"));
        if (element != null) {
          logger.debug("Found 1 element with xpath: " + xpath + "[" + count + "]");
          logger.debug(element.getTagName());
          res.add(element);
          count++;
        } else {
          logger.debug("Finish finding elements at " + xpath + "[" + count + "]");
          done = true;
        }
      } catch (Exception e) {
        done = true;
      }
    }
    return res;
  }
}
