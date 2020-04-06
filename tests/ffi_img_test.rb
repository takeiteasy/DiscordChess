require 'ffi'
require_relative '../ffi_img_lib.rb'

test = ImgLib.load_image "assets/board.png"
test2 = ImgLib.load_image "assets/black_knight.png"
test3 = ImgLib.copy_image test2

ImgLib.blit_image test2, test, 245, 245
ImgLib.blit_image test3, test, 10, 10

ImgLib.save_image test, "test.png"

ImgLib.free_image test
ImgLib.free_image test2
ImgLib.free_image test3