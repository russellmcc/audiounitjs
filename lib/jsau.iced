fs = require 'fs'
path = require 'path'
child_process = require 'child_process'

key = '#PROJNAME'

transform_file = (src, name, target) ->
  await fs.open target, 'w', defer err, target_fd
  throw err if err?

  proc = child_process.spawn 'sed', ['-e', "s/#{key}/#{name}/g", src], {'stdio' : ['ignore', target_fd, 2]}

  await proc.on 'exit', defer code

  throw code if code

  await fs.close target_fd, defer err
  throw err if err?

transform_dir = (dir, name, target) ->

  # create the destination
  await fs.mkdir target, defer err
  throw err if err?

  # read each file from the source.
  await fs.readdir dir, defer err, files
  throw err if err?

  # now tranform each file and subdirectory.
  for file in files
    src_path = path.join dir, file
    target_path = path.join target, file.replace key, name

    await fs.stat src_path, defer err, stats
    throw err if err?

    # if it's a file, transform the file
    if stats.isFile()
      transform_file src_path, name, target_path
    else if stats.isDirectory()
      transform_dir src_path, name, target_path

module.exports = (name) ->
  await fs.realpath (path.join __dirname, '..', 'Template'), defer err, fullpath
  throw err if err?
  target = (path.join process.cwd(), name)

  transform_dir fullpath, name, target