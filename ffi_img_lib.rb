module ImgLib
  extend FFI::Library
  ffi_lib "ffi_img_lib.dylib"
  
  attach_function :load_image, [:pointer, :pointer], :int
  attach_function :copy_image, [:pointer, :pointer], :int
  attach_function :blit_image, [:pointer, :pointer, :int, :int], :int
  attach_function :save_image, [:pointer, :pointer], :int
  attach_function :free_image, [:pointer], :void
end

class Img < FFI::Struct
  layout :width, :int,
         :height, :int,
         :channels, :int,
         :rows, :pointer
end
