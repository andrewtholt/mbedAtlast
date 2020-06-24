# mbedAtlast

// mosi, miso, sck, cs
blockDevice = new SDBlockDevice (PA_7, PA_6, PA_5, PA_8);

On Nucleo-F411RE

PA_7	mosi
PA_6	miso
PA_5	sck
PA_8	cs

on STM32f4-discovery

    SD_MOSI     = PC_3,    
    SD_MISO     = PC_2,    
    SD_SCK      = PB_10,    
    SD_CS       = PE_2,
    
    
