import Control.Arrow
import Data.Bits
import Data.Ix
import Data.Word
import Test.QuickCheck

packBits :: FiniteBits b => [Bool] -> b
packBits xs = foldr (.|.) zeroBits (zipWith shiftL (map f xs) [0..]) where
    f True = bit 0
    f False = zeroBits

unpackBits :: FiniteBits b => b -> [Bool]
unpackBits x = [testBit x i | i <- [0..finiteBitSize x]]

mapBits :: FiniteBits b => (Bool -> Bool -> Bool) -> b -> b -> b
mapBits f x y = packBits (zipWith f (unpackBits x) (unpackBits y))

truthTableToFunction :: (Bool, Bool, Bool, Bool) -> (Bool -> Bool -> Bool)
truthTableToFunction (a,b,c,d) = f where
    f False False = a
    f False  True = b
    f  True False = c
    f  True  True = d

functionToTruthTable f = (a,b,c,d) where
    a = f False False
    b = f False  True
    c = f  True False
    d = f  True  True

allGates = map truthTableToFunction (range ((False,False,False,False),(True,True,True,True)))
allGatePairs = [(f,g) | f <- allGates, g <- allGates]

mapReduce f g b x y = foldr g b (zipWith f (unpackBits x) (unpackBits y))
bruteFunction = [(f,g,b) | (f,g) <- allGatePairs, b <- [False,True]]

main = do
    let i = packBits . unpackBits
    quickCheck (\x -> i x == id (x :: Word64))
    let tablify (f,g,b) = (functionToTruthTable f, functionToTruthTable g, b)
    let bruteNumber = [(x,y) | x <- d, y <- d] where d = [minBound::Word8 .. maxBound]
    mapM (print . tablify) (filter (\(f,g,b) -> all (\(x,y) -> (x < y) == mapReduce f g b x y) bruteNumber) bruteFunction)
