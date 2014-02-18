fs = require 'fs'

# from CoffeeScript cookbook
clone = (obj) ->
  if not obj? or typeof obj isnt 'object'
    return obj

  if obj instanceof Date
    return new Date(obj.getTime())

  if obj instanceof RegExp
    flags = ''
    flags += 'g' if obj.global?
    flags += 'i' if obj.ignoreCase?
    flags += 'm' if obj.multiline?
    flags += 'y' if obj.sticky?
    return new RegExp(obj.source, flags)

  newInstance = new obj.constructor()

  for key of obj
    newInstance[key] = clone obj[key]

  return newInstance

colorToLabel = (color) ->
  switch color
    when 'R' then 'Red'
    when 'G' then 'Green'
    when 'B' then 'Blue'
    when 'K' then 'Black'
    when 'O' then 'Orange'
    when 'W' then 'White'
    when 'N' then 'Brown'
    when 'Y' then 'Yellow'
    else "Unknown"

colorToHex = (color) ->
  switch color
    when 'R' then ['#CA3736', '#000000']
    when 'G' then ['#52C1A5', '#000000']
    when 'B' then ['#0000ff', '#000000']
    when 'K' then ['#1E140F', '#ffffff']
    when 'O' then ['#ED6C3A', '#000000']
    when 'W' then ['#ffffff', '#000000']
    when 'N' then ['#9B7065', '#ffffff']
    when 'Y' then ['#FFFF00', '#000000']
    else "Unknown"

quit = (reason) ->
  console.log "ERROR: #{reason}"
  process.exit()

class Node
  constructor: (@id, @x, @y, @color) ->
    @connections = {}

  clone: ->
    newNode = new Node(@id, @x, @y, @color)
    conns = (c for c of @connections)
    for c in conns
      newNode.connections[c] = 1
    return newNode

  connect: (node) ->
    if not @connections[node.id]
      @connections[node.id] = 1
      node.connect(this)

  disconnect: (node) ->
    if @connections[node.id]
      delete @connections[node.id]
      node.disconnect(this)

  consume: (nodes, node) ->
    conns = (c for c of node.connections)
    for c in conns
      node.disconnect(nodes[c])
      if @id != c
        @connect(nodes[c])

  adjacent: (nodes, color) ->
    conns = (c for c of @connections)
    for c in conns
      if nodes[c].color == color
        return true
    return false

class KamiSolver
  constructor: (@filename, @verboseDepth) ->
    @verboseDepth ?= 1
    @verboseIndents = []
    for i in [0..@verboseDepth+1]
      indents = ""
      indents += "  " for j in [0...i]
      @verboseIndents[i] = indents

    @nodes = {}
    currentID = 1

    # Read in lines, do some sanity checking
    lines = (line for line in fs.readFileSync(@filename, { encoding: 'utf-8' }).split(/\r|\n/) when line.length > 0)
    if lines.length != 10
      quit "#{@filename} has #{lines.length} lines, was expecting 10"
    for line,i in lines
      if line.length != 16
        quit "Line ##{i+1} has #{line.length} chars, was expecting 16"
      if not /^[RGBKOWNY]+$/.test(line)
        quit "Line ##{i+1} has invalid letters"

    # Turn the raw data into a crappy graph
    colorsSeen = {}
    lineNodes = null
    for line,lineNo in lines
      prevLineNodes = lineNodes
      lineNodes = []
      prevNode = null
      for i in [0..15]
        color = line[i]
        colorsSeen[color] = 1
        node = new Node("N#{currentID}", i, lineNo, color)
        currentID++
        @nodes[node.id] = node
        lineNodes.push node
        if prevNode
          prevNode.connect(node)
        if prevLineNodes
          prevLineNodes[i].connect(node)
        prevNode = node

    @colors = (color for color of colorsSeen)

    # ... then make the graph not crappy
    @coalesceNodes(@nodes)

  coalesceNodes: (nodes) ->
    idList = (id for id of nodes)
    for id in idList
      node = nodes[id]
      continue if not node?

      loop
        conns = (c for c of node.connections)
        consumedOne = false
        for connID in conns
          if (node.id < connID) and (node.color == nodes[connID].color)
            node.consume nodes, nodes[connID]
            delete nodes[connID]
            consumedOne = true

        break if not consumedOne
    return

  dump: (nodes, filename) ->
    # Pipe this output to: circo -Tpng -o out.png
    nodes ?= @nodes
    output = "graph G {\n"
    output += "overlap=prism;\n"

    idList = (id for id of nodes)
    for id in idList
      node = nodes[id]
      #console.log "N#{node.id} [label=\"N#{node.id} color #{node.color} (#{node.x}, #{node.y})\" ];"
      colors = colorToHex(node.color)
      output += "N#{node.id} [style=filled; fillcolor=\"#{colors[0]}\"; fontcolor=\"#{colors[1]}\"; label=\"#{node.x}, #{node.y}\" ];\n"
      for c of node.connections
        if c > node.id
          output += "N#{node.id} -- N#{c};\n"

    output += "}\n"

    if filename?
      fs.writeFileSync(filename, output)
    else
      console.log output

  countColors: (idList, nodes) ->
    colorCount = 0
    sawColor = {}
    for id in idList
      if not sawColor[nodes[id].color]
        sawColor[nodes[id].color] = 1
        colorCount++
    return colorCount

  cloneNodes: (nodes) ->
    newNodes = {}
    for id of nodes
      newNodes[id] = nodes[id].clone()
    return newNodes

  solve: (remainingMoves, prevNodes) ->
    if remainingMoves < 0
      return null

    outerLoop = false
    if not prevNodes?
      prevNodes = @nodes
      @totalMoves = remainingMoves
      @attempts = 0
      outerLoop = true
      @dump(prevNodes, "steps/start.txt")

    idList = (id for id of prevNodes)

    colorCount = @countColors(idList, prevNodes)
    if colorCount-1 > remainingMoves
      #console.log "#{colorCount} colors left, only #{remainingMoves} moves left, bailing out"
      return null

    if idList.length == 1
      console.log "SOLVED"
      return []

    if outerLoop
      console.log "Node count: #{idList.length}, color count: #{@colors.length}"

    depth = @totalMoves - remainingMoves
    idCount = 0
    for id in idList
      if depth <= @verboseDepth
        idCount++
        percent = Math.floor(100 * idCount / idList.length)
        console.log "#{@verboseIndents[depth]}#{depth}: #{percent}% complete. (#{idCount} / #{idList.length})"
      for color in @colors
        if (prevNodes[id].color != color) and prevNodes[id].adjacent(prevNodes, color)
          nodes = @cloneNodes(prevNodes)
          @attempts++
          if (@attempts % 100000) == 0
            # vdepth = if depth < @verboseDepth then depth else @verboseDepth
            # console.log "#{@verboseIndents[vdepth]}  attempts: #{@attempts}"
            console.log "                                                              attempts: #{@attempts}"

          x = nodes[id].x
          y = nodes[id].y
          nodes[id].color = color
          @coalesceNodes(nodes)
          moveList = @solve(remainingMoves - 1, nodes)
          if moveList != null
            @dump(nodes, "steps/#{remainingMoves}.txt")
            moveList.unshift {
              x: x
              y: y
              color: colorToLabel(color)
            }
            return moveList

    #loop
    return null

main = ->
  args = process.argv.slice(2)
  if (args.length < 2) or (args.length > 3)
    quit "Syntax: kamisolver filename steps [verboseDepth]"
  filename = args[0]
  steps = args[1]
  if args.length > 2
    verboseDepth = args[2]
  else
    verboseDepth = 0
  fs.mkdir "steps"
  solver = new KamiSolver(filename, verboseDepth)
  moveList = solver.solve(steps)
  console.log "attempts: #{solver.attempts} -- " + JSON.stringify(moveList, null, 2)
  #solver.dump()

main()
