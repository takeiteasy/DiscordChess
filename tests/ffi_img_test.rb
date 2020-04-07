require 'ffi'
require_relative '../ffi_img_lib.rb'

test = Img.new
test2 = Img.new
test3 = Img.new

unless ImgLib.load_image test, "assets/board.png"
  abort
end

unless ImgLib.load_image test2, "assets/black_knight.png"
  abort
end

unless ImgLib.copy_image test2, test3
  abort
end

ImgLib.blit_image test2, test, 245, 245
ImgLib.blit_image test3, test, 10, 10

unless ImgLib.save_image test, "test.png"
  abort
end

ImgLib.free_image test
ImgLib.free_image test2
ImgLib.free_image test3
