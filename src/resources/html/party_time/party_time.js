const app = document.querySelector('#app');

const styles = ['green', 'yellow', 'red', 'blue'];
const degrees = ['0deg', '5deg', '10deg', '-5deg', '-10deg'];
const sizes = ['lowercase', 'uppercase'];

const getRndInteger = (min, max) => {
  return Math.floor(Math.random() * (max - min) ) + min;
}

// add a persistent index to cycle through texts
let textIndex = 0;

const generateTextAnimated = () => {
  // filter out empty or whitespace-only items
  const nonEmptyTexts = (typeof texts !== 'undefined' ? texts : []).filter(
    t => typeof t === 'string' && t.trim().length > 0
  );

  if (nonEmptyTexts.length === 0) {
    // nothing to show, keep current content
    return;
  }

  // pick next item in cycle and advance the index
  const text = nonEmptyTexts[textIndex % nonEmptyTexts.length];
  textIndex = (textIndex + 1) % nonEmptyTexts.length;

  new TextAnimated(text);
}


class TextAnimated {
  constructor(text) {
    this.text = text;
    this.textAnimatedArr = this.getArr(this.text);

    this.render();
  }

  createTag(tagName, content) {
    const tag = document.createElement(tagName);
    const tagInner = document.createElement(tagName);
    tag.className = `animated bounceIn text-item text-${this.getSize()} ${this.getStyle()}`;

    tagInner.textContent = content;
    tagInner.style.transform = `rotate(${this.getDegree()})`;

    tag.appendChild(tagInner);

    return tag;
  }

  getArr(string) {
    return string.split("");
  }

  getStyle() {
    return styles[getRndInteger(0, styles.length)];
  }

  getDegree() {
    return degrees[getRndInteger(0, degrees.length)];
  }

  getSize() {
    return sizes[getRndInteger(0, sizes.length)];
  }

  render() {
    app.innerHTML = "";

    this.textAnimatedArr.map((str) =>
      app.appendChild(this.createTag("div", str))
    );
  }
}

generateTextAnimated();

setInterval(() => {
  generateTextAnimated();
}, 3000);
