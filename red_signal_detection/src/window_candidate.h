int window_num = 322;
int w[322][2][2] = {
{{ 0 , 40 },{ 96 , 176 }},
{{ 0 , 40 },{ 136 , 216 }},
{{ 0 , 40 },{ 176 , 256 }},
{{ 0 , 40 },{ 216 , 296 }},
{{ 0 , 40 },{ 256 , 336 }},
{{ 0 , 40 },{ 296 , 376 }},
{{ 0 , 40 },{ 336 , 416 }},
{{ 0 , 40 },{ 376 , 456 }},
{{ 0 , 40 },{ 416 , 496 }},
{{ 0 , 40 },{ 456 , 536 }},
{{ 20 , 60 },{ 96 , 176 }},
{{ 20 , 60 },{ 136 , 216 }},
{{ 20 , 60 },{ 176 , 256 }},
{{ 20 , 60 },{ 216 , 296 }},
{{ 20 , 60 },{ 256 , 336 }},
{{ 20 , 60 },{ 296 , 376 }},
{{ 20 , 60 },{ 336 , 416 }},
{{ 20 , 60 },{ 376 , 456 }},
{{ 20 , 60 },{ 416 , 496 }},
{{ 20 , 60 },{ 456 , 536 }},
{{ 40 , 80 },{ 96 , 176 }},
{{ 40 , 80 },{ 136 , 216 }},
{{ 40 , 80 },{ 176 , 256 }},
{{ 40 , 80 },{ 216 , 296 }},
{{ 40 , 80 },{ 256 , 336 }},
{{ 40 , 80 },{ 296 , 376 }},
{{ 40 , 80 },{ 336 , 416 }},
{{ 40 , 80 },{ 376 , 456 }},
{{ 40 , 80 },{ 416 , 496 }},
{{ 40 , 80 },{ 456 , 536 }},
{{ 60 , 100 },{ 96 , 176 }},
{{ 60 , 100 },{ 136 , 216 }},
{{ 60 , 100 },{ 176 , 256 }},
{{ 60 , 100 },{ 216 , 296 }},
{{ 60 , 100 },{ 256 , 336 }},
{{ 60 , 100 },{ 296 , 376 }},
{{ 60 , 100 },{ 336 , 416 }},
{{ 60 , 100 },{ 376 , 456 }},
{{ 60 , 100 },{ 416 , 496 }},
{{ 60 , 100 },{ 456 , 536 }},
{{ 80 , 120 },{ 96 , 176 }},
{{ 80 , 120 },{ 136 , 216 }},
{{ 80 , 120 },{ 176 , 256 }},
{{ 80 , 120 },{ 216 , 296 }},
{{ 80 , 120 },{ 256 , 336 }},
{{ 80 , 120 },{ 296 , 376 }},
{{ 80 , 120 },{ 336 , 416 }},
{{ 80 , 120 },{ 376 , 456 }},
{{ 80 , 120 },{ 416 , 496 }},
{{ 80 , 120 },{ 456 , 536 }},
{{ 100 , 140 },{ 96 , 176 }},
{{ 100 , 140 },{ 136 , 216 }},
{{ 100 , 140 },{ 176 , 256 }},
{{ 100 , 140 },{ 216 , 296 }},
{{ 100 , 140 },{ 256 , 336 }},
{{ 100 , 140 },{ 296 , 376 }},
{{ 100 , 140 },{ 336 , 416 }},
{{ 100 , 140 },{ 376 , 456 }},
{{ 100 , 140 },{ 416 , 496 }},
{{ 100 , 140 },{ 456 , 536 }},
{{ 120 , 160 },{ 96 , 176 }},
{{ 120 , 160 },{ 136 , 216 }},
{{ 120 , 160 },{ 176 , 256 }},
{{ 120 , 160 },{ 216 , 296 }},
{{ 120 , 160 },{ 256 , 336 }},
{{ 120 , 160 },{ 296 , 376 }},
{{ 120 , 160 },{ 336 , 416 }},
{{ 120 , 160 },{ 376 , 456 }},
{{ 120 , 160 },{ 416 , 496 }},
{{ 120 , 160 },{ 456 , 536 }},
{{ 140 , 180 },{ 96 , 176 }},
{{ 140 , 180 },{ 136 , 216 }},
{{ 140 , 180 },{ 176 , 256 }},
{{ 140 , 180 },{ 216 , 296 }},
{{ 140 , 180 },{ 256 , 336 }},
{{ 140 , 180 },{ 296 , 376 }},
{{ 140 , 180 },{ 336 , 416 }},
{{ 140 , 180 },{ 376 , 456 }},
{{ 140 , 180 },{ 416 , 496 }},
{{ 140 , 180 },{ 456 , 536 }},
{{ 160 , 200 },{ 96 , 176 }},
{{ 160 , 200 },{ 136 , 216 }},
{{ 160 , 200 },{ 176 , 256 }},
{{ 160 , 200 },{ 216 , 296 }},
{{ 160 , 200 },{ 256 , 336 }},
{{ 160 , 200 },{ 296 , 376 }},
{{ 160 , 200 },{ 336 , 416 }},
{{ 160 , 200 },{ 376 , 456 }},
{{ 160 , 200 },{ 416 , 496 }},
{{ 160 , 200 },{ 456 , 536 }},
{{ 180 , 220 },{ 96 , 176 }},
{{ 180 , 220 },{ 136 , 216 }},
{{ 180 , 220 },{ 176 , 256 }},
{{ 180 , 220 },{ 216 , 296 }},
{{ 180 , 220 },{ 256 , 336 }},
{{ 180 , 220 },{ 296 , 376 }},
{{ 180 , 220 },{ 336 , 416 }},
{{ 180 , 220 },{ 376 , 456 }},
{{ 180 , 220 },{ 416 , 496 }},
{{ 180 , 220 },{ 456 , 536 }},
{{ 200 , 240 },{ 96 , 176 }},
{{ 200 , 240 },{ 136 , 216 }},
{{ 200 , 240 },{ 176 , 256 }},
{{ 200 , 240 },{ 216 , 296 }},
{{ 200 , 240 },{ 256 , 336 }},
{{ 200 , 240 },{ 296 , 376 }},
{{ 200 , 240 },{ 336 , 416 }},
{{ 200 , 240 },{ 376 , 456 }},
{{ 200 , 240 },{ 416 , 496 }},
{{ 200 , 240 },{ 456 , 536 }},
{{ 220 , 260 },{ 96 , 176 }},
{{ 220 , 260 },{ 136 , 216 }},
{{ 220 , 260 },{ 176 , 256 }},
{{ 220 , 260 },{ 216 , 296 }},
{{ 220 , 260 },{ 256 , 336 }},
{{ 220 , 260 },{ 296 , 376 }},
{{ 220 , 260 },{ 336 , 416 }},
{{ 220 , 260 },{ 376 , 456 }},
{{ 220 , 260 },{ 416 , 496 }},
{{ 220 , 260 },{ 456 , 536 }},
{{ 240 , 280 },{ 96 , 176 }},
{{ 240 , 280 },{ 136 , 216 }},
{{ 240 , 280 },{ 176 , 256 }},
{{ 240 , 280 },{ 216 , 296 }},
{{ 240 , 280 },{ 256 , 336 }},
{{ 240 , 280 },{ 296 , 376 }},
{{ 240 , 280 },{ 336 , 416 }},
{{ 240 , 280 },{ 376 , 456 }},
{{ 240 , 280 },{ 416 , 496 }},
{{ 240 , 280 },{ 456 , 536 }},
{{ 0 , 60 },{ 96 , 216 }},
{{ 0 , 60 },{ 126 , 246 }},
{{ 0 , 60 },{ 156 , 276 }},
{{ 0 , 60 },{ 186 , 306 }},
{{ 0 , 60 },{ 216 , 336 }},
{{ 0 , 60 },{ 246 , 366 }},
{{ 0 , 60 },{ 276 , 396 }},
{{ 0 , 60 },{ 306 , 426 }},
{{ 0 , 60 },{ 336 , 456 }},
{{ 0 , 60 },{ 366 , 486 }},
{{ 0 , 60 },{ 396 , 516 }},
{{ 0 , 60 },{ 426 , 546 }},
{{ 0 , 60 },{ 456 , 576 }},
{{ 30 , 90 },{ 96 , 216 }},
{{ 30 , 90 },{ 126 , 246 }},
{{ 30 , 90 },{ 156 , 276 }},
{{ 30 , 90 },{ 186 , 306 }},
{{ 30 , 90 },{ 216 , 336 }},
{{ 30 , 90 },{ 246 , 366 }},
{{ 30 , 90 },{ 276 , 396 }},
{{ 30 , 90 },{ 306 , 426 }},
{{ 30 , 90 },{ 336 , 456 }},
{{ 30 , 90 },{ 366 , 486 }},
{{ 30 , 90 },{ 396 , 516 }},
{{ 30 , 90 },{ 426 , 546 }},
{{ 30 , 90 },{ 456 , 576 }},
{{ 60 , 120 },{ 96 , 216 }},
{{ 60 , 120 },{ 126 , 246 }},
{{ 60 , 120 },{ 156 , 276 }},
{{ 60 , 120 },{ 186 , 306 }},
{{ 60 , 120 },{ 216 , 336 }},
{{ 60 , 120 },{ 246 , 366 }},
{{ 60 , 120 },{ 276 , 396 }},
{{ 60 , 120 },{ 306 , 426 }},
{{ 60 , 120 },{ 336 , 456 }},
{{ 60 , 120 },{ 366 , 486 }},
{{ 60 , 120 },{ 396 , 516 }},
{{ 60 , 120 },{ 426 , 546 }},
{{ 60 , 120 },{ 456 , 576 }},
{{ 90 , 150 },{ 96 , 216 }},
{{ 90 , 150 },{ 126 , 246 }},
{{ 90 , 150 },{ 156 , 276 }},
{{ 90 , 150 },{ 186 , 306 }},
{{ 90 , 150 },{ 216 , 336 }},
{{ 90 , 150 },{ 246 , 366 }},
{{ 90 , 150 },{ 276 , 396 }},
{{ 90 , 150 },{ 306 , 426 }},
{{ 90 , 150 },{ 336 , 456 }},
{{ 90 , 150 },{ 366 , 486 }},
{{ 90 , 150 },{ 396 , 516 }},
{{ 90 , 150 },{ 426 , 546 }},
{{ 90 , 150 },{ 456 , 576 }},
{{ 120 , 180 },{ 96 , 216 }},
{{ 120 , 180 },{ 126 , 246 }},
{{ 120 , 180 },{ 156 , 276 }},
{{ 120 , 180 },{ 186 , 306 }},
{{ 120 , 180 },{ 216 , 336 }},
{{ 120 , 180 },{ 246 , 366 }},
{{ 120 , 180 },{ 276 , 396 }},
{{ 120 , 180 },{ 306 , 426 }},
{{ 120 , 180 },{ 336 , 456 }},
{{ 120 , 180 },{ 366 , 486 }},
{{ 120 , 180 },{ 396 , 516 }},
{{ 120 , 180 },{ 426 , 546 }},
{{ 120 , 180 },{ 456 , 576 }},
{{ 150 , 210 },{ 96 , 216 }},
{{ 150 , 210 },{ 126 , 246 }},
{{ 150 , 210 },{ 156 , 276 }},
{{ 150 , 210 },{ 186 , 306 }},
{{ 150 , 210 },{ 216 , 336 }},
{{ 150 , 210 },{ 246 , 366 }},
{{ 150 , 210 },{ 276 , 396 }},
{{ 150 , 210 },{ 306 , 426 }},
{{ 150 , 210 },{ 336 , 456 }},
{{ 150 , 210 },{ 366 , 486 }},
{{ 150 , 210 },{ 396 , 516 }},
{{ 150 , 210 },{ 426 , 546 }},
{{ 150 , 210 },{ 456 , 576 }},
{{ 180 , 240 },{ 96 , 216 }},
{{ 180 , 240 },{ 126 , 246 }},
{{ 180 , 240 },{ 156 , 276 }},
{{ 180 , 240 },{ 186 , 306 }},
{{ 180 , 240 },{ 216 , 336 }},
{{ 180 , 240 },{ 246 , 366 }},
{{ 180 , 240 },{ 276 , 396 }},
{{ 180 , 240 },{ 306 , 426 }},
{{ 180 , 240 },{ 336 , 456 }},
{{ 180 , 240 },{ 366 , 486 }},
{{ 180 , 240 },{ 396 , 516 }},
{{ 180 , 240 },{ 426 , 546 }},
{{ 180 , 240 },{ 456 , 576 }},
{{ 210 , 270 },{ 96 , 216 }},
{{ 210 , 270 },{ 126 , 246 }},
{{ 210 , 270 },{ 156 , 276 }},
{{ 210 , 270 },{ 186 , 306 }},
{{ 210 , 270 },{ 216 , 336 }},
{{ 210 , 270 },{ 246 , 366 }},
{{ 210 , 270 },{ 276 , 396 }},
{{ 210 , 270 },{ 306 , 426 }},
{{ 210 , 270 },{ 336 , 456 }},
{{ 210 , 270 },{ 366 , 486 }},
{{ 210 , 270 },{ 396 , 516 }},
{{ 210 , 270 },{ 426 , 546 }},
{{ 210 , 270 },{ 456 , 576 }},
{{ 0 , 80 },{ 96 , 256 }},
{{ 0 , 80 },{ 136 , 296 }},
{{ 0 , 80 },{ 176 , 336 }},
{{ 0 , 80 },{ 216 , 376 }},
{{ 0 , 80 },{ 256 , 416 }},
{{ 0 , 80 },{ 296 , 456 }},
{{ 0 , 80 },{ 336 , 496 }},
{{ 0 , 80 },{ 376 , 536 }},
{{ 0 , 80 },{ 416 , 576 }},
{{ 0 , 80 },{ 456 , 616 }},
{{ 40 , 120 },{ 96 , 256 }},
{{ 40 , 120 },{ 136 , 296 }},
{{ 40 , 120 },{ 176 , 336 }},
{{ 40 , 120 },{ 216 , 376 }},
{{ 40 , 120 },{ 256 , 416 }},
{{ 40 , 120 },{ 296 , 456 }},
{{ 40 , 120 },{ 336 , 496 }},
{{ 40 , 120 },{ 376 , 536 }},
{{ 40 , 120 },{ 416 , 576 }},
{{ 40 , 120 },{ 456 , 616 }},
{{ 80 , 160 },{ 96 , 256 }},
{{ 80 , 160 },{ 136 , 296 }},
{{ 80 , 160 },{ 176 , 336 }},
{{ 80 , 160 },{ 216 , 376 }},
{{ 80 , 160 },{ 256 , 416 }},
{{ 80 , 160 },{ 296 , 456 }},
{{ 80 , 160 },{ 336 , 496 }},
{{ 80 , 160 },{ 376 , 536 }},
{{ 80 , 160 },{ 416 , 576 }},
{{ 80 , 160 },{ 456 , 616 }},
{{ 120 , 200 },{ 96 , 256 }},
{{ 120 , 200 },{ 136 , 296 }},
{{ 120 , 200 },{ 176 , 336 }},
{{ 120 , 200 },{ 216 , 376 }},
{{ 120 , 200 },{ 256 , 416 }},
{{ 120 , 200 },{ 296 , 456 }},
{{ 120 , 200 },{ 336 , 496 }},
{{ 120 , 200 },{ 376 , 536 }},
{{ 120 , 200 },{ 416 , 576 }},
{{ 120 , 200 },{ 456 , 616 }},
{{ 160 , 240 },{ 96 , 256 }},
{{ 160 , 240 },{ 136 , 296 }},
{{ 160 , 240 },{ 176 , 336 }},
{{ 160 , 240 },{ 216 , 376 }},
{{ 160 , 240 },{ 256 , 416 }},
{{ 160 , 240 },{ 296 , 456 }},
{{ 160 , 240 },{ 336 , 496 }},
{{ 160 , 240 },{ 376 , 536 }},
{{ 160 , 240 },{ 416 , 576 }},
{{ 160 , 240 },{ 456 , 616 }},
{{ 200 , 280 },{ 96 , 256 }},
{{ 200 , 280 },{ 136 , 296 }},
{{ 200 , 280 },{ 176 , 336 }},
{{ 200 , 280 },{ 216 , 376 }},
{{ 200 , 280 },{ 256 , 416 }},
{{ 200 , 280 },{ 296 , 456 }},
{{ 200 , 280 },{ 336 , 496 }},
{{ 200 , 280 },{ 376 , 536 }},
{{ 200 , 280 },{ 416 , 576 }},
{{ 200 , 280 },{ 456 , 616 }},
{{ 0 , 100 },{ 96 , 296 }},
{{ 0 , 100 },{ 146 , 346 }},
{{ 0 , 100 },{ 196 , 396 }},
{{ 0 , 100 },{ 246 , 446 }},
{{ 0 , 100 },{ 296 , 496 }},
{{ 0 , 100 },{ 346 , 546 }},
{{ 0 , 100 },{ 396 , 596 }},
{{ 50 , 150 },{ 96 , 296 }},
{{ 50 , 150 },{ 146 , 346 }},
{{ 50 , 150 },{ 196 , 396 }},
{{ 50 , 150 },{ 246 , 446 }},
{{ 50 , 150 },{ 296 , 496 }},
{{ 50 , 150 },{ 346 , 546 }},
{{ 50 , 150 },{ 396 , 596 }},
{{ 100 , 200 },{ 96 , 296 }},
{{ 100 , 200 },{ 146 , 346 }},
{{ 100 , 200 },{ 196 , 396 }},
{{ 100 , 200 },{ 246 , 446 }},
{{ 100 , 200 },{ 296 , 496 }},
{{ 100 , 200 },{ 346 , 546 }},
{{ 100 , 200 },{ 396 , 596 }},
{{ 150 , 250 },{ 96 , 296 }},
{{ 150 , 250 },{ 146 , 346 }},
{{ 150 , 250 },{ 196 , 396 }},
{{ 150 , 250 },{ 246 , 446 }},
{{ 150 , 250 },{ 296 , 496 }},
{{ 150 , 250 },{ 346 , 546 }},
{{ 150 , 250 },{ 396 , 596 }},
};
