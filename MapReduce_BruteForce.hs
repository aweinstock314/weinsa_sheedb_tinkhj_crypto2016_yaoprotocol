{-# LANGUAGE FlexibleInstances #-}
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

checkUnpackPackId = do
    let i = packBits . unpackBits
    quickCheck (\x -> i x == id (x :: Word64))

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

bruteMapReduce = do
    let tablify (f,g,b) = (functionToTruthTable f, functionToTruthTable g, b)
    let bruteNumber = [(x,y) | x <- d, y <- d] where d = [minBound::Word8 .. maxBound]
    mapM (print . tablify) (filter (\(f,g,b) -> all (\(x,y) -> (x < y) == mapReduce f g b x y) bruteNumber) bruteFunction)

instance Show (Bool -> Bool -> Bool) where
    show = aux . functionToTruthTable where
        aux (False, True, False, False) = "(<)"
        aux (False, True, True, True) = "(||)"
        aux (False, False, False, True) = "(&&)"
        aux (True, False, False, True) = "xnor"
        aux x = show x

data Gate = Constant Bool | Input Bool Int | Function (Bool->Bool->Bool) Gate Gate deriving Show

{-
mkManualLessThan size = output where
    primitive f i = Function f (Input False i) (Input True i)
    lessthans = map (primitive (<)) [1..size]
    xnors = Constant True : map (primitive ((not .) . xor)) [1..size-1]
    intermediate = zipWith (Function (&&)) lessthans xnors
    output = foldr (Function (||)) (Constant False) intermediate
-}
mkManualLessThan size = output where
    primitive f i = Function f (Input False i) (Input True i)
    lessthans = map (primitive (<)) [size-1,size-2..0]
    xnors = Constant True : map (primitive ((not .) . xor)) [size-1,size-2..0]
    --intermediate1 = Constant True : Constant True : zipWith (Function (&&)) xnors (tail intermediate1)
    intermediate1 = map (\i -> if i < 1 then Constant True else Function (&&) (xnors !! (i-1)) (intermediate1 !! (i-1))) [0..]
    intermediate2 = zipWith3 (\x y z -> (Function (&&) x (Function (&&) y z))) lessthans xnors intermediate1
    output = foldr (Function (||)) (Constant False) intermediate2

eval (Constant b) = (\_ _ -> b)
eval (Input False i) = (\x y -> testBit x i)
eval (Input True i) = (\x y -> testBit y i)
eval (Function f g1 g2) = (\x y -> f (eval g1 x y) (eval g2 x y))

counterExamples n = filter (\(_,_,a,b) -> a /= b) [(x :: Word64,y,eval (mkManualLessThan n) x y,x < y) | x <- [0..2^n-1], y <- [0..2^n-1]]

main = do
    mapM (print . counterExamples) [0..8]
    quickCheck (\x y -> eval (mkManualLessThan 8) (x :: Word8) y == (x < y))
