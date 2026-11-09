// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

// Lays out rendered geometry figures as a multi-page PDF, one page per view.
//
// make_visualisations.py passes the path of a JSON spec as `--input spec=<path>`:
// the title, and for each page its image, text, legend, axes and labels.

#let spec = json(sys.inputs.spec)
#let W = 297mm
#let H = 210mm

#set document(title: spec.title, date: none)
#set page(width: W, height: H, margin: 0pt)
// With these edges text takes no height, so a placed text's y is its baseline.
#set text(font: "DejaVu Sans Mono", top-edge: "baseline", bottom-edge: "baseline")

// The left column is the axes', so that they can never land on the detector.
#let image-region = (x: 0.13 * W, y: 0.14 * H, width: 0.84 * W, height: 0.70 * H)
#let axes-centre = (x: 0.065 * W, y: 0.76 * H)
#let axes-length = 0.033 * W

// Places text with its baseline at a point, aligned horizontally about it.
#let place-text(x, y, body, align: "left") = context {
  let width = measure(body).width
  let shift = if align == "center" { width / 2 } else if align == "right" { width } else { 0pt }
  place(dx: x - shift, dy: y, body)
}

#let place-line(x1, y1, x2, y2, thickness) = place(
  line(start: (x1, y1), end: (x2, y2), stroke: thickness + black),
)

// Draws a round-numbered scale bar about a fifth of the image wide.
#let scale-bar(page, target, pt-per-px) = {
  let wanted = page.crop.width * 0.2 / page.pixels_per_mm
  let magnitude = calc.pow(10.0, calc.floor(calc.log(wanted)))
  let length = magnitude
  for step in (2.0, 5.0) {
    if step * magnitude <= wanted { length = step * magnitude }
  }
  let name = if length >= 1000 { str(length / 1000) + " m" } else { str(length) + " mm" }
  let y = target.y + target.height + 0.015 * H
  let right = target.x + target.width
  let left = right - length * page.pixels_per_mm * pt-per-px
  place-line(left, y, right, y, 2.5pt)
  place-text((left + right) / 2, y + 0.03 * H, text(12pt, name), align: "center")
}

// Labels a subsystem directly, so that reading the figure never relies on colour.
#let direct-label(label, centre, pt-per-px) = {
  let x = centre.x + label.dx * pt-per-px
  let y = centre.y + label.dy * pt-per-px
  let top = y - label.row * 0.042 * H
  place-line(x, y, x, top, 0.7pt)
  place-text(x, top - 0.005 * H, text(10.5pt, label.text), align: label.align)
}

// Draws an axis pointing out of the page as a dot, and into it as a cross.
#let axis-along-sight(toward-viewer) = {
  let (x, y) = (axes-centre.x, axes-centre.y)
  let radius = axes-length * 0.14
  place(dx: x - radius, dy: y - radius, circle(radius: radius, stroke: 1.2pt))
  if toward-viewer {
    let dot = radius * 0.3
    place(dx: x - dot, dy: y - dot, circle(radius: dot, fill: black))
  } else {
    let arm = radius * 0.7
    place-line(x - arm, y - arm, x + arm, y + arm, 1.2pt)
    place-line(x - arm, y + arm, x + arm, y - arm, 1.2pt)
  }
}

// Draws a line with a filled arrowhead at its end.
#let arrow(dx, dy) = {
  let (x, y) = (axes-centre.x, axes-centre.y)
  let (tip-x, tip-y) = (x + dx * axes-length, y + dy * axes-length)
  place-line(x, y, tip-x, tip-y, 1.4pt)
  let size = calc.sqrt(dx * dx + dy * dy)
  let (unit-x, unit-y) = (dx / size, dy / size)
  let head = 0.009 * H
  let (back, side) = (head * 1.6, head * 0.6)
  place(polygon(
    fill: black,
    (tip-x, tip-y),
    (tip-x - unit-x * back - unit-y * side, tip-y - unit-y * back + unit-x * side),
    (tip-x - unit-x * back + unit-y * side, tip-y - unit-y * back - unit-x * side),
  ))
}

// Draws x/y/z arrows as the view sees them.
#let axes(page) = {
  for axis in page.axes {
    let (dx, dy) = (axis.dx, -axis.dy)
    let name = text(12pt, weight: "bold", axis.name)
    if calc.sqrt(dx * dx + dy * dy) < 0.05 {
      axis-along-sight(axis.toward_viewer)
      place-text(
        axes-centre.x + axes-length * 0.3,
        axes-centre.y + axes-length * 0.5,
        name,
        align: "center",
      )
    } else {
      arrow(dx, dy)
      // The baseline sits below the point, so that the letter is centred on it.
      place-text(
        axes-centre.x + dx * axes-length * 1.4,
        axes-centre.y + dy * axes-length * 1.4 + 0.007 * H,
        name,
        align: "center",
      )
    }
  }
}

// Draws swatches and names in rows of up to five, centred under the image.
#let legend(page) = {
  set text(12pt, top-edge: "cap-height", bottom-edge: "baseline")
  let entries = page.legend.map(entry => box(stack(
    dir: ltr,
    spacing: 0.008 * W,
    rect(width: 0.018 * W, height: 0.0099 * W, fill: rgb(entry.colour), stroke: 0.6pt + rgb("#222222")),
    align(horizon, entry.label),
  )))
  place(dx: image-region.x, dy: 0.905 * H, box(width: image-region.width, align(center, grid(
    columns: (auto,) * calc.clamp(entries.len(), 1, 5),
    column-gutter: 0.022 * W,
    row-gutter: 0.012 * H,
    ..entries,
  ))))
}

#for (index, page) in spec.pages.enumerate() {
  if index > 0 { pagebreak() }
  place-text(0.04 * W, 0.05 * H, text(20pt, weight: "bold", page.title))
  place-text(0.04 * W, 0.095 * H, text(14pt, page.subtitle))
  place-text(0.04 * W, 0.975 * H, text(8pt, fill: rgb("#555555"), page.stamp))

  // Fit the crop into the image region, centred, keeping its aspect ratio.
  let pt-per-px = calc.min(
    image-region.width / (page.crop.width * 1pt),
    image-region.height / (page.crop.height * 1pt),
  ) * 1pt
  let (width, height) = (page.crop.width * pt-per-px, page.crop.height * pt-per-px)
  let target = (
    x: image-region.x + (image-region.width - width) / 2,
    y: image-region.y + (image-region.height - height) / 2,
    width: width,
    height: height,
  )
  let size = page.image_size * pt-per-px
  place(dx: target.x, dy: target.y, box(width: width, height: height, clip: true, place(
    dx: -page.crop.left * pt-per-px,
    dy: -page.crop.top * pt-per-px,
    image(page.image, width: size, height: size),
  )))

  if page.scale_bar { scale-bar(page, target, pt-per-px) }
  // The camera looks at the centre of the uncropped image.
  let centre = (
    x: target.x + (page.image_size / 2 - page.crop.left) * pt-per-px,
    y: target.y + (page.image_size / 2 - page.crop.top) * pt-per-px,
  )
  for label in page.labels { direct-label(label, centre, pt-per-px) }
  axes(page)
  legend(page)
}
