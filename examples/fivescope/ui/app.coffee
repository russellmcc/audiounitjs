# config
bgColor = '0,0,0'
traceColor = '10,237,87'
triggerColor = '238,55,103'
gridColor = '237,108,33'  

numScopeData = 400

$ ->
  canvasSize = ($ '#scopeData').width()

  scopeCanvas = ($ '#scopeData')[0]
  ctx = scopeCanvas.getContext '2d'

  ctx.beginPath()
  ctx.arc canvasSize/2, canvasSize/2, canvasSize/2, 0, Math.PI * 2, true
  ctx.clip()

  shouldDrawScope = true

  gridAlpha = .1
  
  drawScope = ->
    return if not shouldDrawScope
    shouldDrawScope = false

    valToX = (val) -> canvasSize/10 + (val/numScopeData) * canvasSize*.8
    valToY = (val) -> ~~(canvasSize/2 - canvasSize/2 * val)

    data = AudioUnit.ScopeData.Get()

    # draw background
    do ->
      ctx.fillStyle = "rgba(#{bgColor},1)"
      ctx.fillRect 0, 0, canvasSize, canvasSize

    # draw the trigger arrow
    do ->
      ctx.beginPath()
      ctx.strokeStyle = "rgb(#{triggerColor})"
      trigY = valToY AudioUnit.Trigger.Get()
      ctx.moveTo 0, trigY
      ctx.lineTo canvasSize/10, trigY
      ctx.moveTo canvasSize/20, trigY
      ctx.lineTo canvasSize/20-canvasSize/100, trigY+5
      ctx.moveTo canvasSize/20, trigY
      ctx.lineTo canvasSize/20-canvasSize/100, trigY-5
      ctx.stroke()

    # draw the grid
    do () ->
      ctx.beginPath()

      if gridAlpha > .05
        tAlpha = gridAlpha*.95 + Math.random()/10
      else
        tAlpha = gridAlpha
        
      ctx.strokeStyle = "rgba(#{gridColor}, #{tAlpha})"
      ctx.lineWidth = 2
      gridWidth = canvasSize * .8
      gridHeight = canvasSize/2
      gridOrigX = (canvasSize - gridWidth)/2
      gridOrigY = (canvasSize - gridHeight)/2

      # outside
      ctx.strokeRect gridOrigX, gridOrigY, gridWidth, gridHeight

      # helpers
      addLineH = (y) ->
        ctx.moveTo gridOrigX, y
        ctx.lineTo gridOrigX + gridWidth, y

      addLineV = (x) ->
        ctx.moveTo x, gridOrigY
        ctx.lineTo x, gridOrigY + gridHeight

      # center lines
      ctx.beginPath()
      addLineH canvasSize/2
      addLineV canvasSize/2
      ctx.stroke()

      # divs
      ctx.lineWidth = 1
      divsPerQuadX = 4
      divsPerQuadY = 3
      gridSpaceX = gridWidth / 2 / (divsPerQuadX + 1)      
      gridSpaceY = gridHeight / 2 / (divsPerQuadY + 1)

      # hatch marks
      hatch = canvasSize/100
      addHatchH = (y) ->
        ctx.moveTo canvasSize/2 - hatch, y
        ctx.lineTo canvasSize/2 + hatch, y
      addHatchV = (x) ->
        ctx.moveTo x, canvasSize/2 - hatch
        ctx.lineTo x, canvasSize/2 + hatch
      
      ctx.beginPath()
      for i in [1..divsPerQuadY]
        addLineH canvasSize/2 - (gridSpaceY * i)
        addLineH canvasSize/2 + (gridSpaceY * i)
        addHatchH canvasSize/2 + (gridSpaceY * i) + gridSpaceY/2
        addHatchH canvasSize/2 - (gridSpaceY * i) - gridSpaceY/2        
      for i in [1..divsPerQuadX]
        addLineV canvasSize/2 - (gridSpaceX * i)
        addLineV canvasSize/2 + (gridSpaceX * i)
        addHatchV canvasSize/2 + (gridSpaceX * i) + gridSpaceX/2
        addHatchV canvasSize/2 - (gridSpaceX * i) - gridSpaceX/2                
      ctx.stroke()


      
    # draw trace
    do ->
      ctx.beginPath()
      ctx.moveTo (valToX 0), valToY data[0]
      for i in [1...numScopeData]
        ctx.lineTo (valToX i), valToY data[i]

      k = 3
      ctx.strokeStyle = 'green'
      for j in [k..1]
        ctx.lineWidth = j
        alpha = Math.pow((k+1-j)/k, 2)
        tAlpha = alpha * .9 + Math.random()/5
        ctx.strokeStyle = "rgba(#{traceColor},#{tAlpha})"
        ctx.stroke()
      
  doDrawScope = ->
    shouldDrawScope = true
    setTimeout drawScope, 0

  drawScope()
  AudioUnit.ScopeData.OnChange = doDrawScope

  syncDial = ->
    ($ '#trigger').val(~~(AudioUnit.Trigger.Get() * 100)).trigger('change')
      
  AudioUnit.Trigger.OnParameterChange = (v) ->
    doDrawScope()
    syncDial()

  ($ '#trigger').dial
    start: -> AudioUnit.Trigger.BeginGesture()
    stop: -> AudioUnit.Trigger.EndGesture()
    change: (v) ->
      AudioUnit.Trigger.Set v / 100
      doDrawScope()
    flatMouse: true
    fgColor: "rgb(#{triggerColor})"
    bgColor: "rgb(#{bgColor})"

  ($ '#grid').dial
    change: (v) ->
      gridAlpha = v / 100
      doDrawScope()
    flatMouse: true
    fgColor: "rgb(#{gridColor})"
    bgColor: "rgb(#{bgColor})"

   syncDial()

